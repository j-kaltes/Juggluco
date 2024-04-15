#include "config.h"
#ifdef USE_HEBREW
#include "jugglucotext.h"
#ifndef WEAROS
//Just some examples
constexpr static std::string_view iwlabels[]={
"מהיר Insuli",
"קרבוהידרה",
"דקסטרו",
"לונג אינסולי",
"אופניים",
"ללכת",
"דם"};
//Just some examples
constexpr static Shortcut_t  shortinit[]= { {"Bread",
        .48},
        {"Currantbun1",
        .56f},
        {"Currantbun2",
        .595f},
        {"Grapes",
        .165f},
        {"FruitYog",
        .058f},
        {"Rice",
        .75f},
        {"Macaroni",
        .65f},
        {"Tomato",
        .03f},
        {"Mexican mix",
        .078f},
        {"OrangeJuice",
        .109f},
        {"SportPowder",
        .873f},
        {"Mix(Carrot)",
        .07f},
        {"Mix mushro",
        .07300000f}};

#endif

jugglucotext iwtext {

.daylabel={ "שמש",
	"ינש",
	"'ג",
	"'ד",
 	"'ה",
	"ישיש",
	"תבש"
	},
.monthlabel={
      "ינואר",
      "פברואר",
      "מרץ",
      "אפריל",
      "מאי" ,             
      "יוני",
       "יולי",
       "אוגוסט",
       "ספטמבר",
      "אוקטובר",
      "נוב",
      "דצמ" },

.scanned="שנסרקו",
.readysecEnable="החיישן מוכן בעוד %d דקות. סרוק שוב כדי להפעיל סטרימינג.",
.readysec="החיישן מוכן בעוד %d דקות.",
.networkproblem="ללא גלוקוז ממראה",
.enablebluetooth="הפעל Bluetooth",
.useBluetoothOff="'השתמש ב-Bluetooth' כבוי",
.noconnectionerror=": אין חיבור",
.stsensorerror=": שגיאת חיישן",
.streplacesensor=": החלפת חיישן?",
.endedformat="%s הפסיק לעבוד. state=%d",
.notreadyformat="%s לא מוכן. state=%d",
#ifndef WEAROS
.median="חציון",
.middle="אמצע",
#endif
.history="היסטוריה",
.historyinfo="פעם ל-15 דקות.\nנזכר בחיישן למשך 8 שעות.\nסריקה מעבירה אותם לתוכנית זו.\nחיישן: ",
.history3info="פעם ל-5 דקות.\nנזכר בחיישן למשך 14 ימים.\nהועבר באמצעות Bluetooth לתוכנית זו.\nחיישן: ",
.sensorstarted= "החיישן התחיל:",
.lastscanned="נסרק אחרון:",
.laststream="זרם אחרון:",
.sensorends="קצה החיישן:",
	.sensorexpectedend="Expected to end: ",
#ifndef WEAROS
.newamount="כמות חדשה",
.averageglucose="גלוקוז ממוצע: ",
.duration="משך: %.1f ימים",
.timeactive="%.1f%% מהזמן הפעיל",
.nrmeasurement="מספר מדידות: %d",
.EstimatedA1C="A1C משוער: %.1f%% (%d mmol/mol)",
.GMI="אינדיקטור לניהול גלוקוז: %.1f%% (%d mmol/mol)",
.SD="SD: %.2f",
.glucose_variability="שונות גלוקוז: %.1f%%",
      .menustr0={
		"System UI           ",
		"Menus",
"שעון",
"חיישן",
"הגדרות",
#if defined(SIBIONICS)
"Sibionics",
#else
"על אודות",
#endif
"סגור",
"עצור אזעקה"
},
.menustr1={
"יְצוּא",
"מַרְאָה",
iwtext.newamount,
"רשימה",
"סטָטִיסטִיקָה",
"דבר",
"לצוף"
},
.menustr2= {
"סריקה אחרונה",
	"סריקות",
	"זרם",
	"היסטוריה",
	"סכומים",
	"ארוחות",
	"מצב כהה"},
.menustr3= {hourminstr,
"חיפוש",
	"תאריך",
	"יום אחורה",
	"יום לאחר מכן",
	"שבוע אחורה",
	"שבוע לאחר מכן"

},
#else
  .amount="כמות",
  .menustr0= {
"מַרְאָה",
"חיישן",
"מצב אפל",
         "הגדרות",
"עצור אזעקה" },
.menustr2= {"תאריך ",hourminstr,"יום אחורה ",iwtext.amount},
#endif

.scanerrors={
	{"שגיאת סריקה (%d)",
			"נסה שוב"},
		{"שגיאת התקנה",
			"?"},
		{"שגיאת עיבוד נתונים",
			"נסה שוב"},
		{"חיישן הפעלה",
			""},
		{"החיישן בהחלט הסתיים",
			""},

	{"חיישן מוכן", "%d דקות"},
		{"373: שגיאת חיישן",
			"נסה שוב מאוחר יותר"},
		{"חיישן חדש אותחל",
			"סרוק שוב כדי להשתמש בו"},
		{"",
			"חוסם מגע במהלך סריקה"},
		{"",
			""},
		{"שגיאת אתחול ספריה",
			"האם חסרות ספריות משותפות?"},
		{"שגיאת אתחול מחלקה",
			"לעשות משהו"},
		{"ההליך לוקח יותר מדי זמן",
			"אני הורג תוכנית"},
		{"365: החלף חיישן",
			"החיישן שלך לא עובד. אנא הסר את החיישן שלך והתחל חיישן חדש."},
		{"368: החלף חיישן",
			"החיישן שלך לא עובד. אנא הסר את החיישן שלך והתחל חיישן חדש."},
		{"",
			""},
		{"שגיאת סריקה",
			"נסה שוב"}},



.libre3scanerror={"FreeStyle Libre 3, שגיאת סריקה", 
	"נסה שוב"},
.libre3wrongID={"שגיאה, מזהה חשבון שגוי?",
	"ציין בהגדרות->Libreview את אותו חשבון המשמש להפעלת החיישן"},
.libre3scansuccess= {"חיישן FreeStyle Libre 3", 
	"ערכי גלוקוז יתקבלו כעת על ידי Juggluco"},
.unknownNFC={"שגיאת סריקת NFC לא מזוהה", "נסה שוב"},
.nolibre3={"חיישן FreeStyle Libre 3","לא נתמך על ידי גרסה זו של Juggluco"},



#ifndef WEAROS
.advancedstart= R"(<h1>מכשיר שונה</h1>
<p>אחת הספריות המשמשות את האפליקציה הזו כוללת BUG שעושה
זה קורס אם הוא מזהה קבצים מסוימים. המכשיר שלך מכיל חלק
קבצים אלה. תוכנית זו מכילה פריצה כדי לעקוף את הבאג הזה, אבל
כנראה שעדיף להפוך את הקבצים האלה לבלתי ניתנים לזיהוי בכמה אחרים
דֶרֶך. ל- Magisk, למשל, יש אפשרות להסתיר שורש בוודאות
יישומים (Magiskhide או Denylist) ולשנות את השם שלו, שניהם
דרושים. במקרה שלך יש לו בעיות עם הקובץ הבא)",
.add_s=true,
.shortinit=shortinit,
.labels=iwlabels,
.checked="checked",
.unchecked="not checked",
.Undetermined="",
.FallingQuickly="Falling quickly",
.Falling="Falling",
.Stable="Changing slowly",
.Rising="Rising",
.RisingQuickly="Rising quickly",
#endif
.receivingpastvalues="Receiving old values"
}


;


void setuseiw() {
  usedtext= &iwtext;
  }
  #endif
