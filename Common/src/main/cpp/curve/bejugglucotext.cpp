#include "jugglucotext.hpp"
#ifdef INJUGGLUCO
#ifndef WEAROS
constexpr static std::string_view belabels[]={ "Вугляв", "Дэкстр", "Хуткі", "Доўгі", "Ровар", "Прагул", "Кроў"};
constexpr static Shortcut_t  beshortinit[]= { {"Bread",
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
        {"Mix(Carrot)",
        .07f},
        {"Mix mushro",
        .07300000f}};
#endif
#endif //INJUGGLUCO

jugglucotext betext {
.daylabel={"Нд","Пн","Аў","Ср","Чц","Пт","Сб"},

#ifdef INJUGGLUCO
.speakdaylabel={
"нядзеля",
"панядзелак",
"аўторак",
"серада",
"чацвер",
"Пятніца",
"субота"
},
.monthlabel={ "Сту", "Лют", "Сак",  "Кра", "Май", "Чэр", "Ліп", "Жні", "Вер", "Кас", "Ліс", "Сне"},
.scanned="Адсканаваць",
.readysecEnable="Датчык гатовы праз %d хвілін. Праскануйце яшчэ раз, каб уключыць струменевую перадачу.",
.readysec="Датчык гатовы праз %d хвілін.",
.networkproblem="Праблема з сеткай?",
.enablebluetooth="Уключыць Bluetooth",
.useBluetoothOff="'Выкарыстоўваць Bluetooth' выкл",
.noconnectionerror=": Няма сувязі",
.stsensorerror=": Памылка датчыка",
.streplacesensor=": Замяніць датчык?",
.endedformat="Датчык %s больш не працуе. state=%d",
.notreadyformat="Датчык %s не гатовы. state=%d",
#ifndef WEAROS
	.median="Медыяна",
	.middle="Сярэдні",
#endif
	.history="Гісторыя",
//	.historyinfo="Раз у 15 хвілін.\nЗапомніў на датчыку 8 гадзін.\nСканаванне перадае іх гэтай праграме.\nДатчык: ",
//	.history3info="Раз у 5 хвілін.\nЗапомніў на датчыку 14 дзён.\nПерададзена па Bluetooth у гэту праграму.\nДатчык: ",
	.sensorstarted= "Датчык запушчаны:",
	.lastscanned="Апошняе сканаванне:",
	.laststream="Апошні паток:",
	.sensorends="Датчык заканчваецца: ",
	.sensorexpectedend="Чакаецца заканчэнне: ",
#endif //INJUGGLUCO
#ifndef WEAROS
#ifdef INJUGGLUCO
	.newamount="Новая сума",
	.averageglucose="Сярэдняя глюкоза: ",
	.duration="Працягласць: %.1f дзён",
	.timeactive="%.1f%% часу актыўны",
	.nrmeasurement="Колькасць вымярэнняў: %d",
	.EstimatedA1C="Ацэначна A1C: %.1f%% (%d mmol/mol)",
	.GMI="Індыкатар кантролю ўзроўню глюкозы (GMI): %.1f%% (%d mmol/mol)",
	.SD="SD: %.2f",
	.glucose_variability="Варыябельнасць глюкозы: %.1f%%",
     .menustr0={
		"System UI      ",
		"Меню",
		"Глядзець",
		"Датчык",
		"Налады",
#if defined(SIBIONICS)
		"Photo",
#else
		"Аб Juggluco",
#endif

		"Зачыніць",
		"Спыніць трывогу"
		},
	.menustr1={
		"Экспарт",
        "Клон",
		betext.newamount,
		"Спіс",
#else
       .statistics=
#endif //INJUGGLUCO

		"Статыстыка",
#ifdef INJUGGLUCO
		"Размаўляць",
		"Парыць      "
		},
	.menustr2= 
    {"Calibrated",
	    "Сканы",
	    "Паток",
	    "Гісторыя",
	    "Сумы",
	    "Ежа",
	    "Dark mode        "},
	.menustr3= {hourminstr,"Пошук","Дата","Дзень назад","Праз дзень","Тыдзень таму","Праз тыдзень"},
#endif //INJUGGLUCO
#else
 .amount="Сума",
 .menustr0= {
	"        Клон",
	"Датчык",
	"Дысплей",
        "Налады",
	"Спыніць трывогу" },
.menustr2= {"Дата",hourminstr,
"Дзень назад              ",betext.amount},
#endif
#ifdef INJUGGLUCO

	.scanerrors={
		{"Памылка сканавання (%d)","Паспрабуй яшчэ"},
		{"Памылка ўстаноўкі","?"},
		{"Памылка апрацоўкі дадзеных","Паспрабуй яшчэ"},
		{"Актывацыя датчыка",""},
		{"Датчык дакладна скончыўся",""},

		{"Датчык гатовы праз","%d хвілін"},
		{"Памылка датчыка (373)","Паўтарыце спробу пазней"},
		{"Новы датчык ініцыялізаваны","Праскануйце яшчэ раз, каб выкарыстоўваць яго"},
		{"","Блакуе дакрананне падчас сканавання"},
		{"",""},
		{"Памылка ініцыялізацыі бібліятэкі","Ці адсутнічаюць агульныя бібліятэкі?"},
		{"Памылка ініцыялізацыі класа","Зрабіце што-небудзь"},
		{"Працэдура займае занадта шмат часу","Я зачыняю праграму"},
		{"Замяніць датчык (365)","Ваш датчык не працуе. Каліласка, выдаліце свой датчык і запусціце новы."},
		{"Замените датчик (368)","Ваш датчык не працуе. Каліласка, выдаліце свой датчык і запусціце новы."},
		{"",""},
		{"Памылка сканавання","Паспрабуй яшчэ"}},

.libre3scanerror={"FreeStyle Libre 3, памылка сканавання", "Паспрабуй яшчэ"},
.libre3wrongID={"Памылка, няправільны ID?","Укажыце ў Налады->Libreview той жа ўліковы запіс, які выкарыстоўваўся для актывацыі датчыка"},
.libre3scansuccess= {"FreeStyle Libre 3 датчык", "Цяпер Juggluco будзе атрымліваць значэнні глюкозы"},
.unknownNFC={"Памылка нераспазнанага сканавання NFC", "Паспрабуй яшчэ"},
.nolibre3={"FreeStyle Libre 3 датчык","Не падтрымліваецца гэтай версіяй Juggluco"},
.libre3zeroID={"Error, zero account ID?",
	R"(Use Left menu->Settings->Exchange data->Libreview->"Get Account ID" to set a non-zero account ID.)"},
.needsandroid8="Needs minimally Android 8"sv,
#ifndef WEAROS
	.advancedstart= R"(<h1>Мадыфікаванае прылада</h1>
<p>У адной з бібліятэк, якія выкарыстоўваюцца гэтым дадаткам, ёсць ПАМЫЛКА, і
   ён выходзіць з ладу, калі выяўляе пэўныя файлы. Ваша прылада змяшчае некаторыя з
   гэтых файлаў. Гэтая праграма змяшчае ўзлом для абыходу гэтай ПАМЫЛКІ, але
   верагодна, лепш зрабіць гэтыя файлы невыяўнымі ў наступны раз. Magisk, напрыклад,
   мае магчымасць схаваць root для пэўных прыкладанняў (Magiskhide or Denylist)
   і змяніць сваё імя, як неабходнае. У вашым выпадку ёсць праблемы з наступным файлам)",
	.add_s=true,
.shortinit=beshortinit,
.labels=belabels,
#endif
#ifndef DONTTALK
.checked="праверана",
.unchecked="не праверана",
.Undetermined="",
.FallingQuickly="Хутка падае",
.Falling="Падзенне",
.Stable="Мяняецца павольна",
.Rising="Падымаецца",
.RisingQuickly="Хутка падымаецца",
//.sibionics="Sibionics",
#endif
        .receivingpastvalues="Атрыманне старых значэнняў",
.receivingdata="Receiving data",
.unsupportedSibionics="Unsupported Sibionics Sensor"sv,
.waitingforconnection="Waiting for connection"sv,
.deleted="Deleted"sv,
.nolocationpermission="Needs location permission"sv,
.nonearbydevicespermission="Needs nearby devices permission"sv

#endif //INJUGGLUCO
,.summarygraph="Зводны графік"sv
,.logdays="Дні"sv
,.unhide="Unhide"sv
		}


		;


addlang(be);
