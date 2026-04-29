#include "jugglucotext.hpp"
#ifdef INJUGGLUCO
#ifndef WEAROS
// 7文字 / labels[] と Shortcut_t::name は ≤11 バイト (UTF-8) に制限される。
// 日本語(BMP)文字は 3 バイト/字 なので最大 3 字。
constexpr static std::string_view labels[]={"糖質",
"葡萄糖",
"速効型",
"持効型",
"自転車",
"散歩",
"血糖"};
// 日本でなじみのある食品の炭水化物量 (g炭水化物 / g食品)。
// 出典: 七訂日本食品標準成分表 (2015) の値を概数化。
// あいまいな食品 (乾麺/茹で麺の差が大きいうどん・そば・パスタ等) は除外し、
// 提供形態が一定で炭水化物量が比較的安定している食品のみを採用しています。
// プリン → カスタードプリン特定; ご飯 = 炊いた白米 (米/精白米とは別)。
constexpr static Shortcut_t  shortinit[]= { {"食パン",
        .47f},          // 食パン (slice bread)
        {"ご飯",
        .37f},          // ご飯 (cooked Japanese white rice)
        {"もち",
        .50f},          // もち (mochi, ready-to-eat rice cake)
        {"大福",
        .51f},          // 大福 (daifuku, mochi with red bean filling)
        {"みかん",
        .11f},          // みかん (mandarin orange, raw)
        {"バナナ",
        .23f},          // バナナ (banana, raw)
        {"りんご",
        .15f},          // りんご (apple, raw)
        {"ぶどう",
        .16f},          // ぶどう (grapes, raw)
        {"トマト",
        .05f},          // トマト (tomato, raw)
        {"牛乳",
        .05f},          // 牛乳 (whole milk)
        {"コーラ",
        .11f},          // コーラ (cola)
        {"アイス",
        .23f},          // アイス (ice cream, regular fat)
        {"角砂糖",
        1.0f}};         // 角砂糖 (sugar cube, ~100% carbohydrate)
#endif
#endif //INJUGGLUCO

extern const jugglucotext jatext;
const jugglucotext jatext {
	.daylabel={"日", "月", "火", "水", "木", "金", "土"},
#ifdef INJUGGLUCO
.speakdaylabel={ "日曜日","月曜日","火曜日","水曜日","木曜日","金曜日","土曜日"},
	.monthlabel={
      "1月",
      "2月",
      "3月",
      "4月",
      "5月"      ,             
      "6月",
       "7月",
       "8月",
       "9月",
      "10月",
      "11月",
      "12月"},

	.scanned="スキャン済み",
	.readysecEnable="あと %d 分でセンサー使用可能。再スキャンでストリーミングを有効化。",
	.readysec="あと %d 分でセンサー使用可能。",
.networkproblem="ミラーから血糖値を受信していません",
.enablebluetooth="Bluetoothを有効化",
.useBluetoothOff="「Bluetoothを使用」がオフ",
.noconnectionerror=": 接続なし",
.stsensorerror=": センサーエラー",
.streplacesensor=": センサーを交換しますか?",
.endedformat="%s が動作を停止しました。state=%d",
.notreadyformat="%s は準備ができていません。state=%d",
#ifndef WEAROS
	.median="中央値",
	.middle="中央",
#endif //WEAROS
	.history="履歴",
//	.historyinfo="15分ごとに 1 回。\nセンサーに 8 時間保存されます。\nスキャンによってこのプログラムへ転送します。\nセンサー: ", 
//	.history3info="5分ごとに 1 回。\nセンサーに 14 日間保存されます。\nBluetoothによってこのプログラムへ転送します。\nセンサー: ",
#ifndef WEAROS
	.sensorstarted= "開始:\t\t\t\t\t\t",
#else
	.sensorstarted= "開始:",
#endif
	.lastscanned="最終スキャン:",
#ifndef WEAROS
	.laststream="最終ストリーム:\t\t",
#else
	.laststream="最終ストリーム:",
#endif
	.sensorends="公式終了日: ",
	.sensorexpectedend="予測終了日: ",
#endif //INJUGGLUCO
#ifndef WEAROS

#ifdef INJUGGLUCO
	.newamount="新規記録",
	.averageglucose="平均血糖値: ",
	.duration="期間: %.1f 日",
	.timeactive="稼働時間の割合: %.1f%%",
	.nrmeasurement="測定数: %d",
	.EstimatedA1C="推定A1C: %.1f%% (%d mmol/mol)",
	.GMI="血糖管理指標: %.1f%% (%d mmol/mol)",
	.SD="標準偏差: %.2f",
	.glucose_variability="血糖変動: %.1f%%",
     .menustr0={
		"システムUI",
		"メニュー",
		"ウォッチ",
		"センサー",
		"設定",

#if defined(SIBIONICS)
		"フォト",
#else
		"情報",
#endif

		"閉じる",
		"アラーム停止"
		},
	.menustr1={
		"エクスポート",
		"ミラー",
		jatext.newamount,
		"一覧", 
#else
       .statistics=
#endif //INJUGGLUCO
		"統計",
#ifdef INJUGGLUCO
		"音声",
		"フロート"
		},
	.menustr2= {"最終スキャン",
	"スキャン",
	"ストリーム",
	"履歴",
	"記録",
	"食事",
	"ダークモード"},
	.menustr3= {hourminstr,
	"検索",
	"日付",
	"1日前",
	"1日後",
	"1週間前",
	"1週間後"},
#endif //INJUGGLUCO
#else
 .amount="記録",
 .menustr0= {
	"ミラー",
	"センサー",
	"表示",
    "設定",
	"アラーム停止" },
.menustr2= {"日付",
hourminstr,
"1日前",
jatext.amount},
#endif //WEAROS
#ifdef INJUGGLUCO
	.scanerrors={
		{"スキャンエラー (%d)", "再試行してください"},
		{"インストールエラー", "?"},
		{"データ処理エラー", "再試行してください"},
		{"センサー起動中", ""},
		{"センサーは完全に終了しました", ""},
		{"センサー使用可能まで", "あと %d 分"},
		{"センサーエラー (373)", "後で再試行してください"},
		{"新しいセンサーを初期化しました", "もう一度スキャンして使用開始"},
		{"", "スキャン中はタッチを無効化"},
		{"", ""},
		{"ライブラリ初期化エラー", "共有ライブラリが不足していますか?"},
		{"クラス初期化エラー", "何か対応してください"},
		{"処理に時間がかかりすぎています", "プログラムを終了します"},
		{"センサーを交換 (365)", "センサーが正常に動作していません。センサーを取り外し、新しいセンサーを起動してください。"},
		{"センサーを交換 (368)", "センサーが正常に動作していません。センサーを取り外し、新しいセンサーを起動してください。"},
		{"", ""},
		{"スキャンエラー", "再試行してください"}},

.libre3scanerror={"FreeStyle Libre 3 スキャンエラー", 
	"再試行してください"},
.libre3wrongID={"エラー: アカウントIDが間違っています",
	R"(センサー起動時と同じIDである必要があります。左メニュー→設定→データ交換→Libreview→「アカウントIDを取得」でLibreviewから取得してください。)"},
.libre3scansuccess= {"FreeStyle Libre 3 センサー", 
	"以後、JugglucoがLibre 3センサーから血糖値を受信します"},
.unknownNFC={"NFCスキャンが認識できませんでした", 
	"再試行してください"},
.nolibre3={"FreeStyle Libre 3 センサー",
	"https://www.juggluco.nl/download.html から正しいバージョンをダウンロードしてください"},
.libre3zeroID={"エラー: アカウントIDが0です",
	R"(左メニュー→設定→データ交換→Libreview→「アカウントIDを取得」を使用して0以外のアカウントIDを設定してください。)"},
.needsandroid8="Android 8 以上が必要です"sv,
#ifndef WEAROS
	.advancedstart= R"(<h1>改造済みデバイス</h1>
<p>このアプリが使用しているライブラリの一つに、特定のファイルを検出するとクラッシュするバグがあります。お使いのデバイスにはこのようなファイルが存在します。本プログラムにはこのバグを回避するための処理が組み込まれていますが、これらのファイルを別の方法で検出されないようにする方が安全です。例えば Magisk であれば、特定のアプリに対して root を隠す機能 (Magiskhide または Denylist) と Magisk 自体の名前変更を組み合わせると効果的で、両方が必要です。お使いのデバイスでは以下のファイルで問題が発生しています)",
	.add_s=true,
.shortinit=shortinit,
.labels=labels,
#endif
#ifndef DONTTALK
.checked="チェック済み",
.unchecked="未チェック",
.Undetermined=""sv,
.FallingQuickly="急速に低下中"sv,
.Falling="低下中"sv,
.Stable="緩やかに変化中"sv,
.Rising="上昇中"sv,
.RisingQuickly="急速に上昇中"sv,
#endif
.receivingpastvalues="過去の値を受信中"sv,
.receivingdata="データ受信中"sv,
.unsupportedSibionics="非対応のSibionicsセンサー"sv,
.waitingforconnection="接続待ち"sv,
.deleted="削除済み"sv,
.nolocationpermission="位置情報の権限が必要です"sv,
.nonearbydevicespermission="「付近のデバイス」の権限が必要です"sv,
#endif //INJUGGLUCO
.summarygraph="サマリーグラフ"sv
,.logdays="日次ログ"sv
,.unhide="表示"sv
		}


		;

#include "logs.hpp"
addlang(ja);
/*
void setuseja() {
LOGAR("setuse ja");
 usedtext= &jatext;
 }*/

//std::unordered_map<uint16_t,const jugglucotext*> langmap;
