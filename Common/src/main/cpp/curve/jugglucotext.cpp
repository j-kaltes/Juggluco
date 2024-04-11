#include "jugglucotext.h"
#ifndef WEAROS
constexpr static std::string_view labels[]={"Fast Insuli",
"Carbohydra",
"Dextro",
"Long Insuli",
"Bike",
"Walk",
"Blood"};
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

extern jugglucotext engtext;
jugglucotext engtext {
	.daylabel={"Sun",
	"Mon",
	"Tue",
	"Wed",
	"Thu",
	"Fri",
	"Sat"},
.speakdaylabel={ "Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"},
	.monthlabel={
      "Jan",
      "Feb",
      "Mar",
      "Apr",
      "May"      ,             
      "Jun",
       "Jul",
       "Aug",
       "Sep",
      "Oct",
      "Nov",
      "Dec"},

	.scanned="Scanned",
	.readysecEnable="Sensor ready in %d minutes. Scan again to enable Streaming.",
	.readysec="Sensor ready in %d minutes.",
.networkproblem="No glucose from mirror",
.enablebluetooth="Enable Bluetooth",
.useBluetoothOff="'Use Bluetooth' off",
.noconnectionerror=": No Connection",
.stsensorerror=": Sensor Error",
.streplacesensor=": Replace Sensor?",
.endedformat="%s stopped working. state=%d",
.notreadyformat="%s not ready. state=%d",
#ifndef WEAROS
	.median="Median",
	.middle="Middle",
#endif
	.history="History",
//	.historyinfo="Once per 15 minutes.\nRemembered on the sensor for 8 hours.\nScanning transfers them to this program.\nSensor: ", 
//	.history3info="Once per 5 minutes.\nRemembered on the sensor for 14 days.\nTransferred by Bluetooth to this program.\nSensor: ",
	.sensorstarted= "Started:",
	.lastscanned="Last scanned:",
	.laststream="Last stream:",
	.sensorends="Ends: ",
#ifndef WEAROS
	.newamount="New Amount",
	.averageglucose="Average glucose: ",
	.duration="Duration: %.1f days",
	.timeactive="%.1f%% of the time active",
	.nrmeasurement="Number of measurements: %d",
	.EstimatedA1C="Estimated A1C: %.1f%% (%d mmol/mol)",
	.GMI="Glucose Management Indicator: %.1f%% (%d mmol/mol)",
	.SD="SD: %.2f",
	.glucose_variability="Glucose variability: %.1f%%",
     .menustr0={
		"System UI        ",
		"Menus",
		"Watch",
		"Sensor",
		"Settings",

#if defined(SIBIONICS)
		"Sibionics",
#else
		"About",
#endif

		"Close",
		"Stop Alarm"
		},
	.menustr1={
		"Export",
		"Mirror",
		engtext.newamount,
		"List", 
		"Statistics",
		"Talk",
		"Float        "
		},
	.menustr2= {"Last Scan",
	"Scans",
	"Stream",
	"History",
	"Amounts",
	"Meals",
	"Dark mode        "},
	.menustr3= {hourminstr,
	"Search",
	"Date",
	"Day back",
	"Day later",
	"Week back",
	"Week later"},
#else
 .amount="Amount",
 .menustr0= {
	"Mirror",
	"Sensor",
	"    Darkmode      ",
        "Settings",
	"Stop Alarm" },
.menustr2= {"Date  ",
hourminstr,
"Day back                ",
engtext.amount},
#endif

	.scanerrors={
		{"Scan Error (%d)",
			"Try again"},
		{"Installation Error",
			"?"},
		{"Data processing Error",
			"Try again"},
		{"Activating Sensor",
			""},
		{"Sensor has definitely ended",
			""},

		{"Sensor ready in",
			"%d minutes"},
		{"Sensor Error (373)",
			"Try again later"},
		{"New Sensor initialized",
			"Scan again to use it"},
		{"",
			"Blocks touch during scanning"},
		{"",
			""},
		{"Library initialization error",
			"Are shared libraries missing?"},
		{"Class initalization error",
			"Do something"},
		{"Procedure takes too long",
			"I kill program"},
		{"Replace Sensor (365)",
			"Your Sensor is not working. Please remove your Sensor and start a new one."},
		{"Replace Sensor (368)",
			"Your Sensor is not working. Please remove your Sensor and start a new one."},
		{"",
			""},
		{"Scan Error",
			"Try again"}},

.libre3scanerror={"FreeStyle Libre 3, Scan error", 
	"Try again"},
.libre3wrongID={"Error, wrong account ID?",
	"Specify in Settings->Libreview the same account used to activate the sensor"},
.libre3scansuccess= {"FreeStyle Libre 3 sensor", 
	"Glucose values will now be received by Juggluco"},
.unknownNFC={"Unrecognized NFC scan Error", 
	"Try again"},
.nolibre3={"FreeStyle Libre 3 sensor",
	"Download the correct version from https://www.juggluco.nl/download.html"},
#ifndef WEAROS
	.advancedstart= R"(<h1>Modified device</h1>
<p>One of the libraries used by this application has a BUG that makes
it crash if it detects certain files. Your device contains some of
these files. This program contains a hack to circumvent this BUG, but
it is probably better to make these files undetectable in some other
way. Magisk, for example, has the option to hide root for certain
applications (Magiskhide or Denylist) and change its own name, both 
are needed. In your case it has problems with the following file)",
	.add_s=true,
.shortinit=shortinit,
.labels=labels,
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

jugglucotext *usedtext= &engtext;
#include "logs.h"
void setuseeng() {
LOGAR("setuse en");
 usedtext= &engtext;
 }
