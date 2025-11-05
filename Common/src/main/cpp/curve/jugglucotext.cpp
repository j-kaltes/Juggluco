#include "jugglucotext.hpp"
#ifdef INJUGGLUCO
#ifndef WEAROS
constexpr static std::string_view labels[]={"Carbohydra",
"Dextro",
"Fast Insuli",
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
#endif //INJUGGLUCO

extern const jugglucotext engtext;
const jugglucotext engtext {
	.daylabel={"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"},
#ifdef INJUGGLUCO
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
#endif //WEAROS
	.history="History",
//	.historyinfo="Once per 15 minutes.\nRemembered on the sensor for 8 hours.\nScanning transfers them to this program.\nSensor: ", 
//	.history3info="Once per 5 minutes.\nRemembered on the sensor for 14 days.\nTransferred by Bluetooth to this program.\nSensor: ",
#ifndef WEAROS
	.sensorstarted= "Started:\t\t\t\t\t\t",
#else
	.sensorstarted= "Started:",
#endif
	.lastscanned="Last scanned:",
#ifndef WEAROS
	.laststream="Last stream:\t\t",
#else
	.laststream="Last stream:",
#endif
	.sensorends="Ends officially: ",
	.sensorexpectedend="Expected end: ",
#endif //INJUGGLUCO
#ifndef WEAROS

#ifdef INJUGGLUCO
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
		"Photo",
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
#else
       .statistics=
#endif //INJUGGLUCO
		"Statistics",
#ifdef INJUGGLUCO
		"Talk",
		"Float        "
		},
	.menustr2= {"Calibrated",
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
#endif //INJUGGLUCO
#else
 .amount="Amount",
 .menustr0= {
	"        Mirror",
	"Sensor",
	"Display",
    "Settings",
	"Stop Alarm" },
.menustr2= {"Date  ",
hourminstr,
"Day back                       ",
engtext.amount},
#endif //WEAROS
#ifdef INJUGGLUCO
	.scanerrors={
		{"Scan Error (%d)", "Try again"},
		{"Installation Error", "?"},
		{"Data processing Error", "Try again"},
		{"Activating Sensor", ""},
		{"Sensor has definitely ended", ""},
		{"Sensor ready in", "%d minutes"},
		{"Sensor Error (373)", "Try again later"},
		{"New Sensor initialized", "Scan again to use it"},
		{"", "Blocks touch during scanning"},
		{"", ""},
		{"Library initialization error", "Are shared libraries missing?"},
		{"Class initalization error", "Do something"},
		{"Procedure takes too long", "I kill program"},
		{"Replace Sensor (365)", "Your Sensor is not working. Please remove your Sensor and start a new one."},
		{"Replace Sensor (368)", "Your Sensor is not working. Please remove your Sensor and start a new one."},
		{"", ""},
		{"Scan Error", "Try again"}},

.libre3scanerror={"FreeStyle Libre 3, Scan error", 
	"Try again"},
.libre3wrongID={"Error, wrong account ID?",
	R"(Should be the same as when activating the sensor. Use Left menu->Settings->Exchange data->Libreview->"Get Account ID" to retrieve it from Libreview.)"},
.libre3scansuccess= {"FreeStyle Libre 3 sensor", 
	"Glucose values will now be received by Juggluco"},
.unknownNFC={"Unrecognized NFC scan Error", 
	"Try again"},
.nolibre3={"FreeStyle Libre 3 sensor",
	"Download the correct version from https://www.juggluco.nl/download.html"},
.libre3zeroID={"Error, zero account ID?",
	R"(Use Left menu->Settings->Exchange data->Libreview->"Get Account ID" to set a non-zero account ID.)"},
.needsandroid8="Needs minimally Android 8"sv,
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
#endif
#ifndef DONTTALK
.checked="checked",
.unchecked="not checked",
.Undetermined=""sv,
.FallingQuickly="Falling quickly"sv,
.Falling="Falling"sv,
.Stable="Changing slowly"sv,
.Rising="Rising"sv,
.RisingQuickly="Rising quickly"sv,
#endif
.receivingpastvalues="Receiving old values"sv,
.receivingdata="Receiving data"sv,
.unsupportedSibionics="Unsupported Sibionics Sensor"sv,
.waitingforconnection="Waiting for connection"sv,
.deleted="Deleted"sv,
.nolocationpermission="Needs location permission"sv,
.nonearbydevicespermission="Needs nearby devices permission"sv,
#endif //INJUGGLUCO
.summarygraph="Summary graph"sv
,.logdays="Daily log"sv
,.unhide="Show"sv
		}


		;

language language_en("en",&engtext);
const jugglucotext *usedtext= &engtext;
#include "logs.hpp"
/*
void setuseeng() {
LOGAR("setuse en");
 usedtext= &engtext;
 }*/
