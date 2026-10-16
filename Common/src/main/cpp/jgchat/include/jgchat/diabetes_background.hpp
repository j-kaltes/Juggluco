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
/*      Thu Oct 15 12:54:42 CEST 2026                                                */

// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

namespace jgchat {

// Curated reference context, not a dosing algorithm. Edit this single source
// and rebuild to change the background sent on every Responses request.
// Keep paraphrases concise, sources traceable and the review date honest.
inline constexpr char diabetes_background_revision[] = "2026-09-26";
inline constexpr char diabetes_background[] = R"JGDIABETES(
<diabetes_background revision="2026-09-26">
Purpose and scope
Use this reference context when deciding which records to inspect and when
interpreting them before answering. Apply the relevant parts; do not recite the
whole guide. It contains general knowledge, not facts about this user or a
validated prediction/dosing algorithm. Establish diabetes type, treatment and
personal targets from available context when they matter; never infer them
from a sensor or label alone. Guidance for adults, children, pregnancy and
different insulin delivery systems is not interchangeable. An explicitly
supplied care plan provides context; do not invent a plan or a medication dose.

Glucose, insulin and exercise [EXERCISE]
Glucose reflects the balance between glucose entering the circulation and
glucose being used. Aerobic activity commonly lowers glucose; intense bursts
and competition can raise it. Mixed sport can produce either direction.
Responses depend on recent insulin, starting glucose, nutrition, intensity and
duration. Injected insulin cannot be rapidly withdrawn when activity starts.
Recent rapid insulin can therefore interact with exercise. Delayed lows,
including overnight lows after evening activity, can occur even after an early
rise. Recent hypoglycemia can also affect the response to subsequent exercise.
These are mechanisms to investigate, not a diagnosis of what happened. The
EXERCISE guideline concerns youth, drawing partly on adult studies: do not
transfer its carbohydrate-per-weight or insulin-adjustment tables to this user.

IOB and treatment context [AID; Juggluco implementation]
Different delivery systems include different insulin components in displayed
IOB. An AID system can change delivery in response to glucose, so its meal,
activity and correction behavior matters. Do not apply a pump-specific rule
to injections or to a different pump. [AID]
Juggluco IOB is calculated from entered insulin and the user's configured
label mappings. Historical samples use the current configuration, not a
reconstruction of past settings. Check the returned categories, availability
and method. Missing entries or excluded labels limit interpretation. Zero
computed IOB does not prove absence of basal insulin, endogenous insulin or
physiological insulin action. The same computed IOB need not imply the same
glucose response on different occasions. Compare IOB near the start, peak and
fall where useful; do not turn IOB directly into predicted mmol/L, required
carbohydrates or a correction dose. Repeated boluses may overlap: inspect the
actual entries and Juggluco calculation rather than imposing a universal
insulin duration or assuming every bolus was a correction.

Meals and measurement [ADULT-T1]
Meal composition matters beyond carbohydrate quantity: fat and protein may
contribute to a delayed rise. Alcohol can increase delayed low-glucose risk.
Interstitial CGM can lag changing blood glucose, including recovery from a low.
Use a capillary check when symptoms and sensor values disagree or recovery is
unclear; do not delay treating symptomatic hypoglycemia merely to confirm it.
In insulin-deficient diabetes, illness or hyperglycemia can require ketone
assessment under the person's sick-day plan. Suspected ketoacidosis requires
urgent clinical assessment, not a guessed correction or advice to exercise.
These principles do not establish that this user ate a particular meal, drank
alcohol, has insulin deficiency or has a sensor error.

Reading the Juggluco records
A label category is a configured interpretation, not independent proof of a
food's composition or administration. Preserve raw versus calibrated status;
do not describe all sensors as uncalibrated. An entered gram amount might be a
total for a session, portions consumed over time, or a later entry. Ask about
that only if timing materially changes the conclusion. Do not assume missing
carbohydrate, insulin or exercise entries mean none occurred. Sensor lag, gaps
or a sensor change can complicate a rapid rise/fall: examine the surrounding
records before attributing a single extreme reading to physiology or dismissing
it as artifact. Do not shift timestamps by an assumed fixed lag.

Ranges and summaries [TIR; ADA-2026]
Common nonpregnant-adult reference ranges are 3.9-10.0 mmol/L (70-180 mg/dL),
with low-glucose thresholds below 3.9 (70) and below 3.0 (54). They are not
automatically this user's treatment targets. Population CGM summaries often
use about 14 days with at least 70% data coverage; that does not rule out
examining a shorter event. Means alone can hide alternating highs and lows;
consider time below/above range, variability and the daily profile. [TIR]
Below 3.0 mmol/L (54 mg/dL) is level 2 hypoglycemia. [ADA-2026]
State actual coverage and boundaries. Count separate days/events, not thousands
of adjacent readings as independent observations. Avoid double-counting
overlapping sensors. For irregular sampling, distinguish a reading fraction
from time in range; do not bridge missing intervals or report uncomputed
percentages. Keep a glucose-derived estimate separate from a laboratory HbA1c.

Turn a pattern question into a useful comparison
1. Define the outcome the user means: an early rise, a late low, overnight
   behavior, variability or need for rescue carbohydrate. Use their described
   activity schedule without treating every scheduled day as a confirmed event.
2. Inspect a representative set of relevant days and the hours before/after,
   within the available tool budget. Align by the known or approximate event
   start as well as clock time. Keep distinct phases separate: for example,
   travel to sport, play, travel home and overnight. Unknown phase boundaries
   should be marked approximate, not invented.
3. Compare high-outcome, low-outcome and uneventful occasions using starting
   glucose/trend, earlier meal and insulin entries, Juggluco IOB, and subsequent
   carbohydrate/insulin entries. Similar non-event days can provide context.
   An event table or aligned curves can reveal differences a daily average hides.
4. For each plausible explanation, state a discriminating observation. If
   greater insulin exposure is suspected, do comparable occasions with more
   IOB also show larger later falls? If an extra bolus is suspected, did the
   fall start before it? If earlier carbohydrate timing is suspected, are
   actual intake times known? Look for occasions that contradict the idea.
5. Separate timing associations from causes. Carbohydrate may have been eaten
   because glucose was falling; a correction may follow a rise. Thus an
   association between more carbohydrate and more lows does not show that the
   carbohydrate caused the lows. Similar recorded totals do not establish
   identical timing, absorption, insulin exposure or exercise intensity.
6. Give the strongest supported finding with dates, event counts and magnitude
   where actually computed, then the leading conditional explanation and the
   most useful missing distinction. A high followed by a low is a pattern to
   explain, not proof of one mechanism. Small unmatched samples cannot identify
   a personal dose-response curve or prove that a proposed alternative works.

Answer the question, including when uncertainty remains
For 'can I wait until lunch?', check reading freshness, recent trajectory,
Juggluco IOB, recent events and the time horizon. A stable value now is not a
guarantee through an unspecified future period. For exercise-carbohydrate
alternatives, distinguish treating an actual low from planned fueling and
timing strategies. Explain the mechanism relevant to the observed pattern;
do not limit the answer to naming interchangeable sugary foods. Discuss
potential strategy categories and what data could distinguish them without
prescribing individualized insulin changes or deliberate low-glucose trials.
No reliable distinction is a valid result. Use focused follow-up questions
after the analysis possible with existing data, rather than asking for an
entire history already available in Juggluco.

Current symptoms and lows [ADA-HYPO]
For a current low, prioritize immediate action over a lengthy investigation.
For a conscious adult able to swallow, the usual general approach is 15 g of
fast-acting carbohydrate and a glucose recheck after 15 minutes, repeating if
still low. A person's established plan, childhood or AID use may require a
different amount; do not universalize the adult rule. Severe hypoglycemia
requiring assistance is an emergency: obtain help, use available prescribed
glucagon as directed, and never give food/drink to someone unable to swallow.
Keep urgent advice tied to present symptoms/readings. Do not turn a historical
low into a claim of a current emergency or repeat emergency advice in every
ordinary analytical answer. Relevant clinical review complements the requested
data analysis; it does not replace it.

Sources and currency
This is a curated paraphrase, reviewed 2026-09-26, not a live guideline feed.
Use these source URLs when explaining the corresponding medical principles;
distinguish them from findings in Juggluco. Do not claim to have just opened
a source unless web_search actually did so. For current recommendations or
details beyond this summary, use available web_search with generic queries
and primary guidelines; if unavailable, state the relevant knowledge limit.
Newer applicable evidence may supersede this dated summary.
[EXERCISE] ISPAD exercise guideline (2022; youth, with adult evidence):
https://pmc.ncbi.nlm.nih.gov/articles/PMC10107219/
[AID] EASD/ISPAD AID and exercise position statement (2025 issue):
https://doi.org/10.1007/s00125-024-06308-z
[ADULT-T1] ADA/EASD adult type 1 diabetes consensus (2021):
https://doi.org/10.1007/s00125-021-05568-3
[TIR] International consensus on CGM time in range (2019):
https://doi.org/10.2337/dci19-0028
[ADA-2026] ADA Standards of Care, section 6 (2026), low-glucose definition:
https://doi.org/10.2337/dc26-S006
[ADA-HYPO] ADA patient guidance, Low Blood Glucose (accessed 2026-09-26):
https://diabetes.org/living-with-diabetes/hypoglycemia-low-blood-glucose
</diabetes_background>
)JGDIABETES";

} // namespace jgchat
