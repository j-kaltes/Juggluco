#include "jugglucotext.hpp"
#ifdef INJUGGLUCO
#ifndef WEAROS
constexpr static std::string_view labels[]={"Uglevod",
"Dextro",
"Tez insulin",
"Bazal ins.",
"Velosiped",
"Yurish",
"Qon"};
constexpr static Shortcut_t  shortinit[]= { {"Non",
        .48},
        {"Mayizb1",
        .56f},
        {"Mayizb2",
        .595f},
        {"Uzum",
        .165f},
        {"Mev yogurt",
        .058f},
        {"Guruch",
        .75f},
        {"Makaron",
        .65f},
        {"Pomidor",
        .03f},
        {"Meks aral",
        .078f},
        {"Apelsharb",
        .109f},
        {"Sport kuk.",
        .873f},
        {"Sabzi aral",
        .07f},
        {"Qo'ziq aral",
        .07300000f}};
#endif
#endif //INJUGGLUCO

extern const jugglucotext uztext;
const jugglucotext uztext {
	.daylabel={"Yak", "Dush", "Sesh", "Chor", "Pay", "Jum", "Shan"},
#ifdef INJUGGLUCO
.speakdaylabel={ "Yakshanba","Dushanba","Seshanba","Chorshanba","Payshanba","Juma","Shanba"},
	.monthlabel={
      "Yan",
      "Fev",
      "Mar",
      "Apr",
      "May",
      "Iyun",
       "Iyul",
       "Avg",
       "Sen",
      "Okt",
      "Noy",
      "Dek"},

	.scanned="Skan qilindi",
	.readysecEnable="Sensor %d daqiqadan keyin tayyor bo'ladi. Oqimni yoqish uchun yana skan qiling.",
	.readysec="Sensor %d daqiqadan keyin tayyor bo'ladi.",
.networkproblem="Mirror'dan glyukoza yo'q",
.enablebluetooth="Bluetoothni yoqing",
.useBluetoothOff="'Bluetoothni ishlatish' o'chirilgan",
.noconnectionerror=": Ulanish yo'q",
.stsensorerror=": Sensor xatosi",
.streplacesensor=": Sensor almashtirilsinmi?",
.endedformat="%s ishlashdan to'xtadi. holat=%d",
.notreadyformat="%s hali tayyor emas. holat=%d",
#ifndef WEAROS
	.median="Mediana",
	.middle="O'rta",
#endif //WEAROS
	.history="Tarix",
#ifndef WEAROS
	.sensorstarted= "Boshlangan:\t\t\t\t\t\t",
#else
	.sensorstarted= "Boshlangan:",
#endif
	.lastscanned="Oxirgi skan:",
#ifndef WEAROS
	.laststream="Oxirgi oqim:\t\t",
#else
	.laststream="Oxirgi oqim:",
#endif
	.sensorends="Rasmiy tugash: ",
    .sensorexpectedend="Kutilgan tugash: ",
#endif //INJUGGLUCO
#ifndef WEAROS

#ifdef INJUGGLUCO
	.newamount="Yangi miqdor",
	.averageglucose="O'rtacha glyukoza: ",
	.duration="Davomiyligi: %.1f kun",
	.timeactive="Vaqtning %.1f%% qismida faol",
	.nrmeasurement="O'lchovlar soni: %d",
	.EstimatedA1C="Taxminiy A1C: %.1f%% (%d mmol/mol)",
	.GMI="Glyukozani boshqarish indikatori: %.1f%% (%d mmol/mol)",
	.SD="SD: %.2f",
	.glucose_variability="Glyukoza o'zgaruvchanligi: %.1f%%",
     .menustr0={
		"Tizim UI",
		"Menyular",
		"Soat",
		"Sensor",
		"Sozlamalar",

#if defined(SIBIONICS)
		"Foto",
#else
		"Haqida",
#endif

		"Yopish",
		"Signalni to'xtatish"
		},
	.menustr1={
		"Eksport",
		"Mirror",
		uztext.newamount,
		"Ro'yxat", 
#else
       .statistics=
#endif //INJUGGLUCO
		"Statistika",
#ifdef INJUGGLUCO
		"Gapirish",
		"Suzuvchi"
		},
	.menustr2= {"Oxirgi skan",
	"Skanlar",
	"Oqim",
	"Tarix",
	"Miqdorlar",
	"Ovqatlar",
	"Qorong'i rejim"},
	.menustr3= {hourminstr,
	"Qidirish",
	"Sana",
	"Oldingi kun",
	"Keyingi kun",
	"Oldingi hafta",
	"Keyingi hafta"},
#endif //INJUGGLUCO
#else
 .amount="Miqdor",
 .menustr0= {
	"        Mirror",
	"Sensor",
	"Ko'rsatish",
    "Sozlamalar",
	"Signalni to'xtatish" },
.menustr2= {"Sana",
hourminstr,
"Oldingi kun",
uztext.amount},
#endif //WEAROS
#ifdef INJUGGLUCO
	.scanerrors={
		{"Skan xatosi (%d)", "Yana urinib ko'ring"},
		{"O'rnatish xatosi", "?"},
		{"Ma'lumotni qayta ishlash xatosi", "Yana urinib ko'ring"},
		{"Sensor faollashtirilmoqda", ""},
		{"Sensor aniq tugagan", ""},
		{"Sensor tayyor bo'ladi", "%d daqiqada"},
		{"Sensor xatosi (373)", "Keyinroq yana urinib ko'ring"},
		{"Yangi sensor ishga tushirildi", "Undan foydalanish uchun yana skan qiling"},
		{"", "Skanlash paytida tegishni bloklaydi"},
		{"", ""},
		{"Kutubxonani ishga tushirish xatosi", "Ulashilgan kutubxonalar yo'qmi?"},
		{"Sinfni ishga tushirish xatosi", "Nimadir qiling"},
		{"Jarayon juda uzoq davom etmoqda", "Dasturni to'xtataman"},
		{"Sensorni almashtiring (365)", "Sensoringiz ishlamayapti. Iltimos, sensorni olib tashlang va yangisini boshlang."},
		{"Sensorni almashtiring (368)", "Sensoringiz ishlamayapti. Iltimos, sensorni olib tashlang va yangisini boshlang."},
		{"", ""},
		{"Skan xatosi", "Yana urinib ko'ring"}},

.libre3scanerror={"FreeStyle Libre 3, skan xatosi", 
	"Yana urinib ko'ring"},
.libre3wrongID={"Xato, Account ID noto'g'rimi?",
	R"(Sensorni faollashtirgandagi bilan bir xil bo'lishi kerak. Uni Libreview'dan olish uchun Chap menyu->Sozlamalar->Ma'lumot almashish->Libreview->"Account ID olish" dan foydalaning.)"},
.libre3scansuccess= {"FreeStyle Libre 3 sensori", 
	"Endi glyukoza qiymatlari Juggluco tomonidan qabul qilinadi"},
.unknownNFC={"Noma'lum NFC skan xatosi", 
	"Yana urinib ko'ring"},
.nolibre3={"FreeStyle Libre 3 sensori",
	"To'g'ri versiyani https://www.juggluco.nl/download.html manzilidan yuklab oling"},
.libre3zeroID={"Xato, Account ID nolmi?",
	R"(Nol bo'lmagan Account ID o'rnatish uchun Chap menyu->Sozlamalar->Ma'lumot almashish->Libreview->"Account ID olish" dan foydalaning.)"},
.needsandroid8="Kamida Android 8 kerak"sv,
#ifndef WEAROS
	.advancedstart= R"(<h1>O'zgartirilgan qurilma</h1>
<p>Bu ilova foydalanadigan kutubxonalardan birida XATO bor: u ma'lum fayllarni aniqlasa, ishdan chiqadi. Qurilmangizda shunday fayllardan ba'zilari bor. Bu dastur ushbu XATOni chetlab o'tish uchun xakdan foydalanadi, lekin bu fayllarni boshqa usul bilan aniqlanmaydigan qilish yaxshiroq bo'lishi mumkin. Masalan, Magisk ayrim ilovalar uchun rootni yashirish (Magiskhide yoki Denylist) va o'z nomini o'zgartirish imkonini beradi; ikkalasi ham kerak. Sizning holatingizda muammo quyidagi fayl bilan bog'liq)",
	.add_s=true,
.shortinit=shortinit,
.labels=labels,
#endif
#ifndef DONTTALK
.checked="belgilangan",
.unchecked="belgilanmagan",
.Undetermined=""sv,
.FallingQuickly="Tez pasaymoqda"sv,
.Falling="Pasaymoqda"sv,
.Stable="Sekin o'zgaradi"sv,
.Rising="Ko'tarilmoqda"sv,
.RisingQuickly="Tez ko'tarilmoqda"sv,
#endif
.receivingpastvalues="Eski qiymatlar qabul qilinmoqda"sv,
.receivingdata="Ma'lumot qabul qilinmoqda"sv,
.unsupportedSibionics="Qo'llab-quvvatlanmaydigan Sibionics sensori"sv,
.waitingforconnection="Ulanish kutilmoqda"sv,
.deleted="O'chirildi"sv,
.nolocationpermission="Joylashuv ruxsati kerak"sv,
.nonearbydevicespermission="Yaqin qurilmalar ruxsati kerak"sv,
#endif //INJUGGLUCO
.summarygraph="Yig'ma grafik"sv
,.logdays="Kunlik jurnal"sv
,.unhide="Ko'rsatish"sv
		}


		;

#include "logs.hpp"
addlang(uz);
/*
void setuseuz() {
LOGAR("setuse uz");
 usedtext= &uztext;
 }*/

//std::unordered_map<uint16_t,const jugglucotext*> langmap;
