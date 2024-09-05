#include "jugglucotext.h"
#ifndef WEAROS
constexpr static std::string_view svlabels[]={
  // max 11 bytes
  // 12345678901
    "Insulin",
    "Kolhydrater",
    "Dextro",
    "Basinsulin",
    "Cykling",
    "Promenad",
    "Blod"
  };
constexpr static Shortcut_t  svshortinit[]= {
  {"Bröd", .48},
  {"Vinbärsbu1", .56f},
  {"vinbärsbu2", .595f},
  {"Druvor", .165f},
  {"Fruktyoghur", .058f},
  {"Ris", .75f},
  {"Pasta", .65f},
  {"Tomat", .03f},
  {"Mexican mix", .078f},
  {"Apelsinjos", .109f},
  {"Sportpulver", .873f},
  {"Mix(morot)", .07f},
  {"Mix svamp", .07300000f}
  };
#endif

jugglucotext svtext {
	.daylabel={"Sön", "Mån", "Tis", "Ons", "Tor", "Fre", "Lör"},
  .speakdaylabel={ "Söndag","Måndag","Tisdag","Onsdag","Torsdag","Fredag","Lördag"},
	.monthlabel={ "Jan", "Feb", "Mar", "Apr", "Maj", "Jun", "Jul", "Aug", "Sep", "Okt", "Nov", "Dec"},
	.scanned="Skannad",
	.readysecEnable="Sensorn redo om %d minuter. Skanna igern för att aktivera strömning.",
	.readysec="Sensorn redo om %d minuter.",
  .networkproblem="Inga värden från spegeln",
  .enablebluetooth="Aktivera Bluetooth",
  .useBluetoothOff="Använder inte Bluethooth",
  .noconnectionerror=": Ingen anslutning",
  .stsensorerror=": Sensorfel",
  .streplacesensor=": Byt sensor?",
  .endedformat="%s slutade fungera. status=%d",
  .notreadyformat="%s är inte redo. status=%d",
#ifndef WEAROS
	.median="Median",
	.middle="Mitten",
#endif
	.history="Historik",
  //	.historyinfo="Var 15:de minut.\nSensorn har minne för de 8 senaste timmarna.\nSkanning överför dem till detta program.\nSensor: ", 
  //	.history3info="Var 5:e minut.\nSensorn har minne för 14 dagar.\nÖverförs via Bluetooth till detta program.\nSensor: ",
	.sensorstarted= "Startad:",
	.lastscanned="Senast skannad:",
	.laststream="Senast strömmade:",
	.sensorends="Avslutas officielt: ",
	.sensorexpectedend="Förväntat avslut: ",
#ifndef WEAROS
	.newamount="Nytt värde",
	.averageglucose="Medelglukos: ",
	.duration="Varaktighet: %.1f dagar",
	.timeactive="Aktiv %.1f%% av tiden",
	.nrmeasurement="Antal mätningar: %d",
	.EstimatedA1C="Beräknat A1C: %.1f%% (%d mmol/mol)",
	.GMI="Glucose Management Indicator: %.1f%% (%d mmol/mol)",
	.SD="SD: %.2f",
	.glucose_variability="Glukos varians: %.1f%%",
     .menustr0={
		"System UI        ",
		"Menyer",
		"Klocka",
		"Sensor",
		"Inställningar",

#if defined(SIBIONICS)
		"Sibionics",
#else
		"Om",
#endif

		"Stäng",
		"Stoppa larm"
		},
	.menustr1={
		"Export",
		"Spegling",
		"Värde",
		"Lista", 
		"Statistik",
		"Tal",
		"Flytande     "
		},
	.menustr2= {
    "Senaste skanning",
    "Skanningar",
    "Strömmade",
    "Historik",
    "Värden",
    "Måltider",
    "Mörkt läge       "},
	.menustr3= {hourminstr,
    "Sök",
    "Datum",
    "Dag tidigare",
    "Dag senare",
    "Vecka tidigare",
    "Vecka senare"},
#else
   .amount="Värde",
   .menustr0= {
      "Spegling",
      "Sensor",
      "  Mörkt läge      ",
      "Inställningar",
      "Stoppa larm"
    },
    .menustr2= {
      "Datum  ",
      hourminstr,
      "Dag tillbaka            ",
      "Värde"
    },
#endif

	.scanerrors={
		{"Skanningsfel (%d)", "Försök igen"},
		{"Installationsfel", "?"},
		{"Databehandlingsfel", "Försök igen"},
		{"Aktiverar Sensor", ""},
		{"Sensorn har definitivt avslutats", ""},
		{"Sensorn redo om ", "%d minuter"},
		{"Sensorfel (373)", "Försök igen senare"},
		{"Ny sensor initierad", "Skanna igen för att använda den"},
		{"", "Blockerar pekning under scanning"},
		{"", ""},
		{"Biblioteksinitieringsfel", "Saknas delade bibliotek?"},
		{"Klassinitieringsfel", "Gör något"},
		{"Proceduren tar för lång tid", "Jag avslutar programmet"},
		{"Byt sensor (365)", "Sensorn fungerar inte. Ta bort den och starta en ny"},
		{"Byt Sensor (368)", "Sensorn fungerar inte. Ta bort den och starta en ny"},
		{"", ""},
		{"Skanningsfel", "Försök igen"}},

.libre3scanerror={
  "FreeStyle Libre 3, skanningsfel",
  "Försök igen"},
.libre3wrongID={
  "Fel! Kontrollera konto-id",
	"Ange samma konto som används för att aktivera sensorn i Inställningar->Libreview"},
.libre3scansuccess= {
  "FreeStyle Libre 3 sensor", 
	"Glukosvärden kommer nu att tas emot av Juggluco"},
.unknownNFC={
  "Okänt NFC-skanningfel", 
	"Försök igen"},
.nolibre3={
  "FreeStyle Libre 3 sensor",
	"Ladda ner rätt version från https://www.juggluco.nl/download.html"},
#ifndef WEAROS
	.advancedstart= R"(<h1>Modifierad enhet</h1>
<p>Ett av biblioteken som används av denna applikation har en BUG som
gör att den kraschar om den upptäcker vissa filer. Din enhet
innehåller några av dessa filer. Det här programmet innehåller ett
hack för att kringgå denna BUG, men det är förmodligen bättre att
göra dessa filer oupptäckbara på något annat sätt. Appen Magisk har till
exempel möjlighet att dölja root för vissa applikationer (Magiskhide
eller Denylist) och ändra sitt eget namn, båda behövs. I ditt fall
är det problem med följande fil)",
	.add_s=true,
  .shortinit=svshortinit,
  .labels=svlabels,
  .checked="Aktiv",
  .unchecked="Inaktiv",
  .Undetermined="",
  .FallingQuickly="Snabbt fallande",
  .Falling="Fallande",
  .Stable="Ändras långsamt",
  .Rising="Stigande",
  .RisingQuickly="Snabbt stigane",
#endif
  .receivingpastvalues="Ta emot gamla värden"
	};

extern void setusesv() ;
void setusesv() {
  usedtext= &svtext;
  }
