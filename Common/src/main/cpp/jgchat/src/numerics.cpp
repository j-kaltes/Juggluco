/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2(+), Libre 3(+), Dexcom G7/ONE+,        */
/*      Sibionics GS1Sb and GS3, Accu-Chek SmartGuide, CareSens Air and              */
/*      Aidex X sensors.                                                             */
/*                                                                                   */
/*      Copyright (C) 2021 Jaap Korthals Altes <jaapkorthalsaltes@gmail.com>         */
/*                                                                                   */
/*      Juggluco is free software: you can redistribute it and/or modify             */
/*      it under the terms of the GNU General Public License as published            */
/*      by the Free Software Foundation, either version 3 of the License, or         */
/*      (at your option) any later version.                                          */
/*                                                                                   */
/*      Juggluco is distributed in the hope that it will be useful, but              */
/*      WITHOUT ANY WARRANTY; without even the implied warranty of                   */
/*      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                         */
/*      See the GNU General Public License for more details.                         */
/*                                                                                   */
/*      You should have received a copy of the GNU General Public License            */
/*      along with Juggluco. If not, see <https://www.gnu.org/licenses/>.            */
/*                                                                                   */
/*      Thu Oct 15 12:53:38 CEST 2026                                                */

// SPDX-License-Identifier: GPL-3.0-or-later
#include "jgchat/ui_messages.hpp"
#include "jgchat/numerics.hpp"
#include "jgchat/plot.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <numeric>
#include <set>
#include <stdexcept>

namespace jgchat {
namespace {
using Vector = std::vector<double>;
using Matrix = std::vector<Vector>;
constexpr std::size_t max_rows = 10000, max_features = 24, max_targets = 24, max_output_rows = 20000;
constexpr const char* interval_warning =
    "Intervals use held-out absolute errors separately per target (split-conformal order statistic). "
    "Nominal coverage relies on exchangeable calibration/future errors; serial dependence, changed behavior, "
    "missing inputs and distribution shift can invalidate it. Test coverage is an observed fraction, not a guarantee. "
    "These are pointwise intervals, not simultaneous coverage for a whole curve. "
    "A fitted association is not a causal treatment effect or a validated dosing algorithm.";
struct Budget {
    const std::atomic_bool& cancel;
    uint64_t work = 0;
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    void check(uint64_t cost = 1) {
        if (cancel) throw jgchat::UiError(jgchat::UiCode::request_cancelled);
        work += cost;
        if (work > 60000000 || std::chrono::steady_clock::now() >= end)
            throw jgchat::UiError(jgchat::UiCode::numerical_work_exceeds_10_seconds_60_million_operations_use_fewer_rows_features_targe);
    }
};
[[noreturn]] void bad(UiCode message) { throw UiError(message); }
void shape(const Json& args, std::initializer_list<const char*> keys) {
    if (!args.is_object() || args.size() != keys.size()) bad(UiCode::invalid_numerical_tool_arguments);
    for (auto key : keys) if (!args.contains(key)) bad(UiCode::missing_numerical_tool_argument);
}
std::string text(const Json& v, std::size_t max = 128) {
    if (!v.is_string()) bad(UiCode::numerical_column_name_argument_must_be_text);
    const auto s = v.get<std::string>();
    if (s.empty() || s.size() > max || s.find('\0') != std::string::npos) bad(UiCode::invalid_numerical_column_name_length);
    return s;
}
bool numeric(const Json& v) { return v.is_number() && std::isfinite(v.get<double>()) && std::abs(v.get<double>()) <= 1e100; }
double real(const Json& v) {
    if (!numeric(v)) bad(UiCode::numerical_data_must_be_finite_json_numbers_within_1e100_prepare_csv_tsv_with_explicit);
    return v.get<double>();
}
double finite(double v) { if (!std::isfinite(v) || std::abs(v) > 1e100) bad(UiCode::numerical_overflow_or_unstable_model_rescale_inputs_or_simplify_the_model); return v; }
std::vector<std::string> names(const Json& a, std::size_t min, std::size_t max) {
    if (!a.is_array() || a.size() < min || a.size() > max) bad(UiCode::invalid_numerical_feature_target_count);
    std::set<std::string> seen; std::vector<std::string> out;
    for (const auto& v : a) {
        auto s = text(v);
        if (!seen.insert(s).second) bad(UiCode::duplicate_numerical_column);
        out.push_back(std::move(s));
    }
    return out;
}
std::size_t column(const AnalysisTable& t, const std::string& name) {
    const auto it = std::find(t.columns.begin(), t.columns.end(), name);
    if (it == t.columns.end()) throw UiError(UiMessage::detail(UiCode::missing_numerical_column, name));
    return it - t.columns.begin();
}
std::vector<std::size_t> columns(const AnalysisTable& t, const std::vector<std::string>& names) {
    std::vector<std::size_t> out;
    for (const auto& name : names) out.push_back(column(t,name));
    return out;
}
void table_ok(const AnalysisTable& t) {
    if (t.columns.empty() || t.columns.size() > 64 || t.rows.empty() || t.rows.size() > max_rows)
        bad(UiCode::numerical_tables_require_1_10000_rows_and_at_most_64_columns);
    for (const auto& row : t.rows) if (!row.is_array() || row.size() != t.columns.size()) bad(UiCode::invalid_numerical_table_row);
}
Vector vector(const Json& value, std::size_t size) {
    if (!value.is_array() || value.size() != size) bad(UiCode::invalid_saved_numerical_vector);
    Vector out; for (const auto& v : value) out.push_back(real(v)); return out;
}
struct Row { std::size_t index; double at, label_end; Vector x, y; Json baseline; };
struct Model {
    std::string method;
    std::vector<std::string> features, targets;
    Vector mean, scale, minimum, maximum, ymean;
    Matrix coefficients, x, y;
    std::size_t neighbors = 0;
    double diagonal_ratio = 1;
};
Vector scaled(const Model& m, const Vector& x) {
    Vector z(x.size());
    for (std::size_t j = 0; j < x.size(); ++j) z[j] = finite((x[j]-m.mean[j])/m.scale[j]);
    return z;
}
// Column-pivoted Householder QR of [standardized X; sqrt(lambda) I]. Centering
// X and Y on training means gives an unpenalized intercept. Never form X'X.
void regress(Model& m, const std::vector<Row>& rows, double lambda, Budget& budget) {
    const std::size_t p = m.features.size(), q = m.targets.size(), n = rows.size() + (lambda > 0 ? p : 0);
    m.coefficients.assign(q,Vector(p,0));
    if (!p) return;
    Matrix a(n,Vector(p)), b(n,Vector(q));
    for (std::size_t i = 0; i < rows.size(); ++i) {
        budget.check(p+q);
        a[i] = scaled(m,rows[i].x);
        for (std::size_t j = 0; j < q; ++j) b[i][j] = rows[i].y[j]-m.ymean[j];
    }
    if (lambda > 0) for (std::size_t j = 0; j < p; ++j) a[rows.size()+j][j] = std::sqrt(lambda);
    std::vector<std::size_t> permutation(p); std::iota(permutation.begin(),permutation.end(),0);
    double largest = 0, smallest = 1e100;
    for (std::size_t k = 0; k < p; ++k) {
        budget.check((n-k)*(p-k+q)*6);
        double norm = -1; std::size_t pivot = k;
        for (std::size_t j = k; j < p; ++j) {
            double candidate = 0;
            for (std::size_t i = k; i < n; ++i) candidate = std::hypot(candidate,a[i][j]);
            if (candidate > norm) { norm = candidate; pivot = j; }
        }
        largest = std::max(largest,norm);
        if (!std::isfinite(norm) || norm <= std::max(1e-12,largest*1e-10))
            bad(UiCode::rank_deficient_or_ill_conditioned_fit_remove_redundant_constant_features_or_use_ridge);
        smallest = std::min(smallest,norm);
        if (pivot != k) {
            for (auto& row : a) std::swap(row[k],row[pivot]);
            std::swap(permutation[k],permutation[pivot]);
        }
        const double alpha = -std::copysign(norm,a[k][k]);
        Vector v(n-k); v[0] = a[k][k]-alpha;
        double vnorm = std::abs(v[0]);
        for (std::size_t i = k+1; i < n; ++i) { v[i-k] = a[i][k]; vnorm = std::hypot(vnorm,v[i-k]); }
        for (auto& value : v) value /= vnorm;
        for (std::size_t j = k; j < p; ++j) {
            long double dot = 0;
            for (std::size_t i = k; i < n; ++i) dot += static_cast<long double>(v[i-k])*a[i][j];
            for (std::size_t i = k; i < n; ++i) a[i][j] -= 2*v[i-k]*static_cast<double>(dot);
        }
        for (std::size_t j = 0; j < q; ++j) {
            long double dot = 0;
            for (std::size_t i = k; i < n; ++i) dot += static_cast<long double>(v[i-k])*b[i][j];
            for (std::size_t i = k; i < n; ++i) b[i][j] = finite(b[i][j]-2*v[i-k]*static_cast<double>(dot));
        }
        a[k][k] = alpha;
        for (std::size_t i = k+1; i < n; ++i) a[i][k] = 0;
    }
    m.diagonal_ratio = smallest/largest;
    for (std::size_t target = 0; target < q; ++target) {
        Vector beta(p);
        for (std::size_t k = p; k-- > 0;) {
            long double value = b[k][target];
            for (std::size_t j = k+1; j < p; ++j) value -= static_cast<long double>(a[k][j])*beta[j];
            beta[k] = finite(static_cast<double>(value/a[k][k]));
        }
        for (std::size_t j = 0; j < p; ++j) m.coefficients[target][permutation[j]] = beta[j];
    }
}
Vector predict(const Model& m, const Vector& input, Budget& budget) {
    auto z = scaled(m,input);
    if (m.method != "knn") {
        budget.check(m.features.size()*m.targets.size()+1);
        Vector out = m.ymean;
        for (std::size_t t = 0; t < out.size(); ++t) {
            long double value = out[t];
            for (std::size_t j = 0; j < z.size(); ++j) value += static_cast<long double>(z[j])*m.coefficients[t][j];
            out[t] = finite(static_cast<double>(value));
        }
        return out;
    }
    budget.check(m.x.size()*(z.size()+12));
    std::vector<std::pair<double,std::size_t>> distances; distances.reserve(m.x.size());
    for (std::size_t i = 0; i < m.x.size(); ++i) {
        double distance = 0;
        for (std::size_t j = 0; j < z.size(); ++j) distance = std::hypot(distance,z[j]-m.x[i][j]);
        distances.emplace_back(finite(distance),i);
    }
    std::partial_sort(distances.begin(),distances.begin()+m.neighbors,distances.end());
    Vector out(m.targets.size());
    for (std::size_t t = 0; t < out.size(); ++t) {
        long double sum = 0;
        for (std::size_t k = 0; k < m.neighbors; ++k) sum += m.y[distances[k].second][t];
        out[t] = finite(static_cast<double>(sum/m.neighbors));
    }
    return out;
}
unsigned outside(const Model& m, const Vector& input) {
    unsigned count = 0;
    for (std::size_t j = 0; j < input.size(); ++j) if (input[j] < m.minimum[j] || input[j] > m.maximum[j]) ++count;
    return count;
}
Json save_model(const Model& m) {
    Json result{{"schema",1},{"method",m.method},{"features",m.features},{"targets",m.targets},
        {"mean",m.mean},{"scale",m.scale},{"minimum",m.minimum},{"maximum",m.maximum},
        {"target_mean",m.ymean},{"coefficients_standardized",m.coefficients},{"neighbors",m.neighbors},
        {"training_x_standardized",m.x},{"training_y",m.y},{"qr_diagonal_ratio",m.diagonal_ratio}};
    Json equations = Json::array();
    if (m.method != "knn") for (std::size_t t = 0; t < m.targets.size(); ++t) {
        Json weights = Json::object(); long double intercept = m.ymean[t];
        for (std::size_t j = 0; j < m.features.size(); ++j) {
            const double coefficient = finite(m.coefficients[t][j]/m.scale[j]);
            weights[m.features[j]] = coefficient; intercept -= static_cast<long double>(coefficient)*m.mean[j];
        }
        equations.push_back({{"target",m.targets[t]},{"intercept",finite(static_cast<double>(intercept))},{"coefficients",weights}});
    }
    result["equations_original_units"] = std::move(equations);
    return result;
}
Model load_model(const Json& data) {
    if (!data.is_object() || data.value("schema",0) != 1) bad(UiCode::unsupported_saved_numerical_model);
    Model m; m.method = text(data.at("method"));
    if (m.method != "linear" && m.method != "ridge" && m.method != "knn") bad(UiCode::unsupported_saved_numerical_method);
    m.features = names(data.at("features"),0,max_features); m.targets = names(data.at("targets"),1,max_targets);
    const auto p = m.features.size(), q = m.targets.size();
    m.mean = vector(data.at("mean"),p); m.scale = vector(data.at("scale"),p);
    m.minimum = vector(data.at("minimum"),p); m.maximum = vector(data.at("maximum"),p);
    m.ymean = vector(data.at("target_mean"),q);
    for (std::size_t j = 0; j < p; ++j) if (m.scale[j] <= 0 || m.minimum[j] > m.maximum[j]) bad(UiCode::invalid_saved_feature_scaling);
    if (m.method == "knn") {
        const auto& x = data.at("training_x_standardized"); const auto& y = data.at("training_y");
        if (!p || !x.is_array() || !y.is_array() || x.empty() || x.size() > max_rows || x.size() != y.size()) bad(UiCode::invalid_saved_neighbors);
        const auto& k = data.at("neighbors");
        if (!k.is_number_integer() || k.get<int64_t>() < 1 || k.get<int64_t>() > 100 || static_cast<uint64_t>(k.get<int64_t>()) > x.size()) bad(UiCode::invalid_saved_neighbor_count);
        m.neighbors = k.get<std::size_t>();
        for (std::size_t i = 0; i < x.size(); ++i) { m.x.push_back(vector(x[i],p)); m.y.push_back(vector(y[i],q)); }
    } else {
        const auto& coefficients = data.at("coefficients_standardized");
        if (!coefficients.is_array() || coefficients.size() != q) bad(UiCode::invalid_saved_coefficients);
        for (const auto& row : coefficients) m.coefficients.push_back(vector(row,p));
    }
    return m;
}
struct Metrics {
    std::size_t n = 0, covered = 0, compared = 0;
    long double absolute = 0, squared = 0, bias = 0, mean_squared = 0, base_squared = 0, compared_squared = 0;
    void add(double actual, double prediction, double mean, const Json& baseline, const Json& radius) {
        const long double error = prediction-actual, mean_error = mean-actual;
        ++n; absolute += std::abs(error); squared += error*error; bias += error; mean_squared += mean_error*mean_error;
        if (!radius.is_null() && std::abs(error) <= radius.get<double>()) ++covered;
        if (numeric(baseline)) {
            const long double e = baseline.get<double>()-actual;
            ++compared; base_squared += e*e; compared_squared += error*error;
        }
    }
    Json json(const Json& radius) const {
        auto average = [](long double v,std::size_t count) -> Json { return count ? Json(finite(static_cast<double>(v/count))) : Json(nullptr); };
        auto rms = [](long double v,std::size_t count) -> Json { return count ? Json(finite(static_cast<double>(std::sqrt(v/count)))) : Json(nullptr); };
        return {{"rows",n},{"mae",average(absolute,n)},{"rmse",rms(squared,n)},{"bias_prediction_minus_actual",average(bias,n)},
            {"training_mean_baseline_rmse",rms(mean_squared,n)},{"baseline_comparison_rows",compared},
            {"baseline_rmse",rms(base_squared,compared)},{"model_rmse_on_baseline_rows",rms(compared_squared,compared)},
            {"interval_coverage",n && !radius.is_null() ? Json(double(covered)/n) : Json(nullptr)},
            {"interval_width",radius.is_null() ? Json(nullptr) : Json(2*radius.get<double>())}};
    }
};
void output_ok(const Json& data) {
    if (data.at("rows").size() > max_output_rows || data.dump().size() > 2*1024*1024)
        bad(UiCode::numerical_output_exceeds_20000_rows_2_mib_reduce_rows_or_targets_no_partial_result_sa);
}
Json text_schema(std::size_t max) { return {{"type","string"},{"minLength",1},{"maxLength",max}}; }
Json nullable_text() { return {{"type",Json::array({"string","null"})},{"minLength",1},{"maxLength",128}}; }
Json function(const char* name,const char* description,Json props) {
    Json required = Json::array(); for (auto it=props.begin();it!=props.end();++it) required.push_back(it.key());
    return {{"type","function"},{"name",name},{"description",description},{"strict",true},
        {"parameters",{{"type","object"},{"properties",props},{"required",required},{"additionalProperties",false}}}};
}
}

Json numerical_tool_definitions() {
    auto source = function("unused","unused",{{"id",text_schema(33)},
        {"path",{{"type","string"},{"maxLength",512}}},{"format",{{"type","string"},{"enum",Json::array({"json","csv","tsv"})}}}})["parameters"];
    auto columns = Json{{"type","array"},{"maxItems",24},{"items",text_schema(128)}};
    auto targets = columns; targets["minItems"] = 1;
    Json result = Json::array();
    result.push_back(function("juggluco_fit",
        "Fit reusable generic numeric models, not a preset glucose predictor. Prepare a saved numeric table with juggluco_query: join inputs, past lags, interactions/polynomials and explicit future targets. Features/targets are column names (up to 24 each). Methods: linear (pivoted QR), ridge (standardized training inputs; lambda>0), knn (uniform k nearest standardized training inputs). Unused lambda/neighbors must be null. Empty features gives an intercept-only linear/ridge baseline. Chronological split by numeric order_column: <train_before trains; [train_before,calibrate_before) calibrates intervals; >=calibrate_before tests. label_end_column gives when ALL targets were available: crossing training/calibration boundaries is purged; null uses origin and cannot guard forecast leakage. Choose cutoffs with enough separate days; never use future-derived predictors. Fits/scaling use training ONLY. coverage=null disables intervals; otherwise 0.5-0.99 requests per-target held-out absolute-error intervals; time-series coverage is empirical, not guaranteed. Reports test MAE/RMSE/bias/coverage and training-mean baseline; baseline_column optionally compares persistence/another precomputed baseline. Missing/nonfinite rows are counted, not zeroed. Saves exact model, args, source and calibration/test predictions; at most 10000 input rows, 20000 output rows/2 MiB, 10 s/60M operations. Can repeat different chronological cutoffs for rolling evaluation; leave a final untouched test period when selecting models.",
        {{"title",text_schema(160)},{"source",source},{"features",columns},{"targets",targets},
        {"order_column",text_schema(128)},{"label_end_column",nullable_text()},
        {"train_before",{{"type","number"}}},{"calibrate_before",{{"type","number"}}},
        {"method",{{"type","string"},{"enum",Json::array({"linear","ridge","knn"})}}},
        {"lambda",{{"type",Json::array({"number","null"})},{"minimum",0},{"maximum",1000000}}},
        {"neighbors",{{"type",Json::array({"integer","null"})},{"minimum",1},{"maximum",100}}},
        {"coverage",{{"type",Json::array({"number","null"})},{"minimum",0.5},{"maximum",0.99}}},
        {"baseline_column",nullable_text()}}));
    result.push_back(function("juggluco_predict",
        "Apply a saved juggluco_fit model to another saved numeric table containing its same features and order column. No fitting or updated uncertainty calibration occurs. Saves one row per input/target with prediction, lower/upper, origin/order, target and outside-training-range flags. Missing inputs produce flagged null predictions, never zero. Intervals remain pointwise with the model's nominal coverage and original test metrics; refresh data after new records/pen scans and verify feature/target units, definitions and calibration still match training. No clinical validity or causal effect is implied. Use SQL to assign actual future plot times from each target's defined horizon, then juggluco_plot_table.",
        {{"title",text_schema(160)},{"model_id",text_schema(33)},{"source",source}}));
    auto series = function("unused","unused",{{"label",text_schema(60)},{"column",text_schema(128)}})["parameters"];
    result.push_back(function("juggluco_plot_table",
        "Plot numeric columns directly from one saved table, without transcribing points. x_column is numeric or Unix seconds; series names y columns. line/scatter/bar or band. For band, first THREE series must be center, lower, upper with aligned gaps and increasing x; shading is between lower/upper. Up to five further lines allowed. NULL y values stay gaps. max_gap=null disables extra splitting; a positive value inserts gaps when x jumps farther (seconds for time). Rows must be ordered with strictly increasing x for line/band: prepare/filter/pivot with SQL. At most 8 series and 1000 total points including inserted gaps; no silent sampling. Title/labels/provenance must identify calculated forecasts or descriptive estimates, units, coverage and assumptions accurately. Rendering does not validate a statistical confidence claim. Four plots per answer.",
        {{"title",text_schema(100)},{"source",source},{"x_column",text_schema(128)},
        {"x_label",text_schema(60)},{"y_label",text_schema(60)},
        {"x_type",{{"type","string"},{"enum",Json::array({"number","time"})}}},
        {"kind",{{"type","string"},{"enum",Json::array({"line","scatter","bar","band"})}}},
        {"provenance",text_schema(160)},{"max_gap",{{"type",Json::array({"number","null"})},{"exclusiveMinimum",0}}},
        {"series",{{"type","array"},{"minItems",1},{"maxItems",8},{"items",series}}}}));
    return result;
}

Json fit_analysis_model(const AnalysisTable& table, const Json& args, const std::atomic_bool& cancel) {
    Budget budget{cancel}; budget.check(); table_ok(table);
    shape(args,{"title","source","features","targets","order_column","label_end_column","train_before","calibrate_before","method","lambda","neighbors","coverage","baseline_column"});
    (void)text(args.at("title"),160);
    Model m; m.method = text(args.at("method"));
    m.features = names(args.at("features"),0,max_features); m.targets = names(args.at("targets"),1,max_targets);
    const auto p = m.features.size(), q = m.targets.size();
    for (const auto& f : m.features) if (std::find(m.targets.begin(),m.targets.end(),f) != m.targets.end()) bad(UiCode::a_target_cannot_also_be_a_predictor);
    const auto xcols = columns(table,m.features), ycols = columns(table,m.targets);
    const auto order_name = text(args.at("order_column"));
    const auto order = column(table,order_name);
    const auto label_end = args["label_end_column"].is_null() ? order : column(table,text(args["label_end_column"]));
    const auto baseline = args["baseline_column"].is_null() ? table.columns.size() : column(table,text(args["baseline_column"]));
    if (std::find(ycols.begin(),ycols.end(),baseline) != ycols.end()) bad(UiCode::a_target_cannot_serve_as_its_own_baseline);
    const double train_before = real(args["train_before"]), calibrate_before = real(args["calibrate_before"]);
    if (train_before >= calibrate_before) bad(UiCode::train_before_must_be_less_than_calibrate_before);
    const bool intervals = !args["coverage"].is_null();
    const double coverage = intervals ? real(args["coverage"]) : 0;
    if (intervals && (coverage < .5 || coverage > .99)) bad(UiCode::coverage_must_be_0_5_0_99_or_null);
    double lambda = 0;
    if (m.method == "ridge") {
        lambda = real(args["lambda"]);
        if (lambda <= 0 || lambda > 1e6 || !args["neighbors"].is_null()) bad(UiCode::ridge_needs_lambda_in_0_1000000_and_neighbors_null);
    } else if (m.method == "knn") {
        if (!p || !args["lambda"].is_null() || !args["neighbors"].is_number_integer() ||
            args["neighbors"].get<int64_t>() < 1 || args["neighbors"].get<int64_t>() > 100) bad(UiCode::knn_needs_predictors_neighbors_1_100_and_lambda_null);
        m.neighbors = args["neighbors"].get<std::size_t>();
    } else if (m.method != "linear" || !args["lambda"].is_null() || !args["neighbors"].is_null()) bad(UiCode::linear_needs_lambda_null_and_neighbors_null);
    std::vector<Row> train, calibration, test;
    std::size_t invalid = 0, purged_train = 0, purged_calibration = 0;
    for (std::size_t i = 0; i < table.rows.size(); ++i) {
        budget.check(p+q+1); const auto& cells = table.rows[i];
        bool valid = numeric(cells[order]) && numeric(cells[label_end]);
        for (auto col : xcols) valid = valid && numeric(cells[col]);
        for (auto col : ycols) valid = valid && numeric(cells[col]);
        if (!valid) { ++invalid; continue; }
        Row r{i,real(cells[order]),real(cells[label_end]),{},{},baseline < cells.size() && numeric(cells[baseline]) ? cells[baseline] : Json(nullptr)};
        if (r.label_end < r.at) bad(UiCode::label_end_column_precedes_its_origin_order_correct_target_availability_times);
        for (auto c : xcols) r.x.push_back(real(cells[c]));
        for (auto c : ycols) r.y.push_back(real(cells[c]));
        if (r.at < train_before) {
            if (r.label_end >= train_before) ++purged_train; else train.push_back(std::move(r));
        } else if (r.at < calibrate_before) {
            if (r.label_end >= calibrate_before) ++purged_calibration; else calibration.push_back(std::move(r));
        } else test.push_back(std::move(r));
    }
    if (train.size() < 3 || (m.method == "linear" && train.size() <= p+1) || m.neighbors > train.size())
        bad(UiCode::too_few_complete_training_rows_after_purging_linear_needs_more_than_features_1_every_);
    if ((calibration.size()+test.size())*q > max_output_rows) bad(UiCode::evaluation_exceeds_20000_rows_use_fewer_rows_or_targets);
    const auto by_time = [](const Row& a,const Row& b) { return a.at != b.at ? a.at < b.at : a.index < b.index; };
    for (auto* rows : {&train,&calibration,&test}) std::sort(rows->begin(),rows->end(),by_time);
    m.mean.assign(p,0); m.scale.assign(p,0); m.minimum.assign(p,1e100); m.maximum.assign(p,-1e100); m.ymean.assign(q,0);
    Vector moments(p,0);
    for (std::size_t i = 0; i < train.size(); ++i) {
        budget.check(p+q+1);
        for (std::size_t j = 0; j < p; ++j) {
            const double delta = train[i].x[j]-m.mean[j]; m.mean[j] += delta/double(i+1);
            moments[j] += delta*(train[i].x[j]-m.mean[j]);
            m.minimum[j] = std::min(m.minimum[j],train[i].x[j]); m.maximum[j] = std::max(m.maximum[j],train[i].x[j]);
        }
        for (std::size_t t = 0; t < q; ++t) m.ymean[t] += (train[i].y[t]-m.ymean[t])/double(i+1);
    }
    for (std::size_t j = 0; j < p; ++j) { m.scale[j] = finite(std::sqrt(std::max(0.0,moments[j])/train.size())); if (m.scale[j] == 0) m.scale[j] = 1; }
    if (m.method == "knn") for (const auto& row : train) { budget.check(p+q); m.x.push_back(scaled(m,row.x)); m.y.push_back(row.y); }
    else regress(m,train,lambda,budget);
    Matrix cal_predictions; std::vector<Vector> errors(q);
    for (const auto& row : calibration) {
        auto prediction = predict(m,row.x,budget);
        for (std::size_t t = 0; t < q; ++t) errors[t].push_back(std::abs(prediction[t]-row.y[t]));
        cal_predictions.push_back(std::move(prediction));
    }
    Json radii = Json::array();
    const auto rank = intervals ? static_cast<std::size_t>(std::ceil((calibration.size()+1)*coverage)) : 0;
    const bool calibrated = intervals && calibration.size() >= 10 && rank <= calibration.size();
    for (auto& e : errors) {
        budget.check(e.size()+1);
        if (!calibrated) radii.push_back(nullptr);
        else { std::nth_element(e.begin(),e.begin()+rank-1,e.end()); radii.push_back(e[rank-1]); }
    }
    Json rows = Json::array(), metrics = Json::array();
    std::vector<Metrics> cal_metrics(q), test_metrics(q);
    for (unsigned split = 0; split < 2; ++split) {
        const auto& selected = split ? test : calibration;
        for (std::size_t i = 0; i < selected.size(); ++i) {
            const auto& row = selected[i]; const auto prediction = split ? predict(m,row.x,budget) : cal_predictions[i];
            for (std::size_t t = 0; t < q; ++t) {
                (split ? test_metrics[t] : cal_metrics[t]).add(row.y[t],prediction[t],m.ymean[t],row.baseline,radii[t]);
                const auto radius = radii[t];
                rows.push_back({{"row",row.index},{"order",row.at},{"label_end",row.label_end},{"split",split ? "test" : "calibration"},
                    {"target",m.targets[t]},{"actual",row.y[t]},{"prediction",prediction[t]},
                    {"lower",radius.is_null() ? Json(nullptr) : Json(finite(prediction[t]-radius.get<double>()))},
                    {"upper",radius.is_null() ? Json(nullptr) : Json(finite(prediction[t]+radius.get<double>()))},
                    {"baseline",row.baseline},{"features_outside_training_range",outside(m,row.x)}});
            }
        }
    }
    for (std::size_t t = 0; t < q; ++t) metrics.push_back({{"target",m.targets[t]},
        {"calibration",cal_metrics[t].json(radii[t])},{"test",test_metrics[t].json(radii[t])}});
    auto bounds = [](const std::vector<Row>& r) -> Json {
        std::set<double> origins; double latest_label = -1e100;
        for (const auto& row : r) { origins.insert(row.at); latest_label = std::max(latest_label,row.label_end); }
        return {{"rows",r.size()},{"distinct_origins",origins.size()},{"first_origin",r.empty() ? Json(nullptr) : Json(r.front().at)},
            {"last_origin",r.empty() ? Json(nullptr) : Json(r.back().at)},{"last_label_end",r.empty() ? Json(nullptr) : Json(latest_label)}};
    };
    Json result{{"status","ok"},{"model",save_model(m)},{"rows",std::move(rows)},{"metrics",metrics},
        {"order_column",order_name},{"arguments",args},
        {"split",{{"training",bounds(train)},{"calibration",bounds(calibration)},{"test",bounds(test)},
            {"invalid_rows_dropped",invalid},{"purged_training_rows",purged_train},{"purged_calibration_rows",purged_calibration},
            {"input_rows",table.rows.size()},{"label_availability_checked",!args["label_end_column"].is_null()}}},
        {"interval",{{"status",!intervals ? "not_requested" : calibrated ? "calibrated" : "insufficient_calibration_rows"},
            {"nominal_coverage",args["coverage"]},{"calibration_rows",calibration.size()},{"quantile_rank",intervals ? Json(rank) : Json(nullptr)},
            {"absolute_error_radius_by_target",radii},{"interpretation",interval_warning}}},
        {"evaluation",test.empty() ? "no_test_rows" : "chronological_holdout"},
        {"limitations","No automatic proof of data coverage, feature availability, independent samples or causal effects. SQL preparation defines units, lags, horizons, calibration and missingness. Avoid future information in predictors; label_end only guards target overlap at split boundaries. Overlapping windows are dependent. Retrospectively imported/backdated entries do not prove they were known at the historical forecast time. Model selection using test results consumes that test set; reserve another untouched period. Repeat cutoffs for rolling evaluation. Refresh/rebuild after source corrections."}};
    budget.check(); output_ok(result); return result;
}

Json predict_analysis_model(const AnalysisTable& table, const Json& saved, const std::atomic_bool& cancel) {
    Budget budget{cancel}; budget.check(); table_ok(table);
    const auto m = load_model(saved.at("model"));
    const auto xcols = columns(table,m.features);
    const auto order = column(table,text(saved.at("order_column")));
    const auto& interval = saved.at("interval"); const auto& radii = interval.at("absolute_error_radius_by_target");
    if (!radii.is_array() || radii.size() != m.targets.size()) bad(UiCode::invalid_saved_uncertainty_calibration);
    for (const auto& r : radii) if (!r.is_null() && real(r) < 0) bad(UiCode::invalid_saved_interval_radius);
    if (table.rows.size()*m.targets.size() > max_output_rows) bad(UiCode::prediction_exceeds_20000_output_rows_reduce_input_rows);
    Json rows = Json::array(); std::size_t invalid = 0;
    for (std::size_t i = 0; i < table.rows.size(); ++i) {
        budget.check(); const auto& cells = table.rows[i];
        bool valid = numeric(cells[order]); Vector x;
        for (auto c : xcols) { if (!numeric(cells[c])) valid = false; else x.push_back(real(cells[c])); }
        if (!valid) ++invalid;
        const auto values = valid ? predict(m,x,budget) : Vector();
        for (std::size_t t = 0; t < m.targets.size(); ++t) {
            const auto& radius = radii[t];
            rows.push_back({{"row",i},{"order",numeric(cells[order]) ? cells[order] : Json(nullptr)},
                {"target",m.targets[t]},{"status",valid ? "ok" : "missing_or_nonnumeric_input"},
                {"prediction",valid ? Json(values[t]) : Json(nullptr)},
                {"lower",valid && !radius.is_null() ? Json(finite(values[t]-radius.get<double>())) : Json(nullptr)},
                {"upper",valid && !radius.is_null() ? Json(finite(values[t]+radius.get<double>())) : Json(nullptr)},
                {"features_outside_training_range",valid ? Json(outside(m,x)) : Json(nullptr)}});
        }
    }
    Json result{{"status","ok"},{"rows",std::move(rows)},{"input_rows",table.rows.size()},{"invalid_input_rows",invalid},
        {"interval",interval},{"model_test_metrics",saved.at("metrics")},
        {"interpretation","Calculated estimates, not observed values. Original frozen model/scaling/intervals; no automatic retraining. Match original units, target horizons and feature definitions. Outside-range count only checks marginal ranges and cannot certify familiar joint conditions. Refresh inputs after new or corrected records."}};
    budget.check(); output_ok(result); return result;
}

Json plot_analysis_table(const AnalysisTable& table, const Json& args, const std::atomic_bool& cancel) {
    Budget budget{cancel}; budget.check(); table_ok(table);
    shape(args,{"title","source","x_column","x_label","y_label","x_type","kind","provenance","max_gap","series"});
    const auto kind = text(args.at("kind"));
    const auto xcol = column(table,text(args.at("x_column")));
    const bool gap_check = !args["max_gap"].is_null();
    const double max_gap = gap_check ? real(args["max_gap"]) : 0;
    if (gap_check && max_gap <= 0) bad(UiCode::max_gap_must_be_positive_or_null);
    if (!args["series"].is_array() || args["series"].empty() || args["series"].size() > 8) bad(UiCode::plot_needs_1_8_column_series);
    Json series = Json::array(); std::vector<std::size_t> ycols;
    for (const auto& s : args["series"]) {
        shape(s,{"label","column"}); ycols.push_back(column(table,text(s["column"])));
        series.push_back({{"label",text(s["label"],240)},{"points",Json::array()}});
    }
    std::size_t count = 0; double previous = 0; bool first = true;
    for (const auto& row : table.rows) {
        budget.check(); const auto x = real(row[xcol]);
        if (!first && (kind == "line" || kind == "band") && x <= previous) bad(UiCode::line_band_table_x_must_increase_strictly_order_filter_pivot_it_with_sql);
        if (!first && gap_check && x-previous > max_gap) {
            for (auto& s : series) s["points"].push_back({{"x",previous+(x-previous)/2},{"y",nullptr}});
            count += series.size();
        }
        for (std::size_t j = 0; j < ycols.size(); ++j) {
            const auto& y = row[ycols[j]];
            if (!y.is_null()) (void)real(y);
            series[j]["points"].push_back({{"x",x},{"y",y}});
        }
        count += series.size(); if (count > 1000) bad(UiCode::saved_table_plot_exceeds_1000_total_points_explicitly_reduce_aggregate_in_sql);
        previous = x; first = false;
    }
    Json spec; for (auto key : {"title","x_label","y_label","x_type","kind","provenance"}) spec[key] = args.at(key);
    spec["series"] = std::move(series);
    auto result = xy_plot(spec,true); result["source"] = args["source"]; result["input_rows"] = table.rows.size();
    return result;
}
}
