// Azerbaijani translation of Juggluco's native interface strings.
// Add this file alongside jugglucotext.cpp; keep the original English file.
#include "jugglucotext.hpp"
#ifdef INJUGGLUCO
#ifndef WEAROS
// These labels are at most 12 UTF-8 bytes each.
constexpr static std::string_view labels[]={
    "Karbohidrat", "Dekstro", "Tez insulin", "UzunInsulin",
    "Velosiped", "Gəzinti", "Qan"
};
// Shortcut_t::name is char[12]: at most 11 UTF-8 bytes plus NUL.
constexpr static Shortcut_t shortinit[]={
    {"Çörək", .48},
    {"Kiş.bulka1", .56f},
    {"Kiş.bulka2", .595f},
    {"Üzüm", .165f},
    {"Meyvə yoq.", .058f},
    {"Düyü", .75f},
    {"Makaron", .65f},
    {"Pomidor", .03f},
    {"Meks.qar.", .078f},
    {"Port.şirə", .109f},
    {"İdman tozu", .873f},
    {"Kök qar.", .07f},
    {"Göb.qar.", .07300000f}
};
#endif
#endif

extern const jugglucotext aztext;
const jugglucotext aztext {
    .daylabel={"Baz", "B.e.", "Ç.a.", "Çər", "C.a.", "Cüm", "Şən"},
#ifdef INJUGGLUCO
    .speakdaylabel={"Bazar", "Bazar ertəsi", "Çərşənbə axşamı", "Çərşənbə", "Cümə axşamı", "Cümə", "Şənbə"},
    .monthlabel={"Yan", "Fev", "Mar", "Apr", "May", "İyn", "İyl", "Avq", "Sen", "Okt", "Noy", "Dek"},
    .scanned="Skan edildi",
    .readysecEnable="Sensor %d dəqiqədən sonra hazır olacaq. Axını aktivləşdirmək üçün yenidən skan edin.",
    .readysec="Sensor %d dəqiqədən sonra hazır olacaq.",
    .networkproblem="Güzgüdən qlükoza göstəricisi gəlmir",
    .enablebluetooth="Bluetooth-u aktiv edin",
    .useBluetoothOff="'Bluetooth-dan istifadə et' söndürülüb",
    .noconnectionerror=": Bağlantı yoxdur",
    .stsensorerror=": Sensor xətası",
    .streplacesensor=": Sensor dəyişdirilsin?",
    .endedformat="%s işləməyi dayandırdı. Vəziyyət=%d",
    .notreadyformat="%s hazır deyil. Vəziyyət=%d",
#ifndef WEAROS
    .median="Median",
    .middle="Orta",
#endif
    .history="Tarixçə",
#ifndef WEAROS
    .sensorstarted="Başlama vaxtı:\t\t\t\t\t\t",
#else
    .sensorstarted="Başlama vaxtı:",
#endif
    .lastscanned="Son skan:",
#ifndef WEAROS
    .laststream="Son axın:\t\t",
#else
    .laststream="Son axın:",
#endif
    .sensorends="Rəsmi bitmə vaxtı: ",
    .sensorexpectedend="Gözlənilən bitmə vaxtı: ",
#endif // INJUGGLUCO
#ifndef WEAROS
#ifdef INJUGGLUCO
    .newamount="Yeni miqdar",
    .averageglucose="Orta qlükoza: ",
    .duration="Müddət: %.1f gün",
    .timeactive="Vaxtın %.1f%%-də aktiv",
    .nrmeasurement="Ölçmə sayı: %d",
    .EstimatedA1C="uGMI ≈ Təxmini A1C: %.1f%% (%d mmol/mol)",
    .GMI="Qlükoza idarəetmə göstəricisi: %.1f%% (%d mmol/mol)",
    .SD="Standart sapma: %.2f",
    .glucose_variability="Qlükoza dəyişkənliyi: %.1f%%",
    .menustr0={
        "ChatGPT", "Menyular", "Saat", "Sensor", "Tənzimləmələr",
#if defined(SIBIONICS)
        "Foto",
#else
        "Haqqında",
#endif
        "Bağla", "Siqnalı dayandır"
    },
    .menustr1={
        "İxrac et", "Güzgü", aztext.newamount, "Siyahı",
#else
    .statistics=
#endif // INJUGGLUCO
        "Statistika",
#ifdef INJUGGLUCO
        "Səsləndir", "Üzən pəncərə"
    },
    .menustr2={
        "Android interfeysi", "Skanlar", "Axın", "Tarixçə",
        "Miqdarlar", "Yeməklər", "Qaranlıq rejim"
    },
    .menustr3={
        hourminstr, "Axtar", "Tarix", "Bir gün geri", "Bir gün irəli",
        "Bir həftə geri", "Bir həftə irəli"
    },
#endif // INJUGGLUCO
#else
    .amount="Miqdar",
    .menustr0={"        Güzgü", "Sensor", "Ekran", "Tənzimləmələr", "Siqnalı dayandır"},
    .menustr2={"Tarix", hourminstr, "Bir gün geri", aztext.amount},
#endif // WEAROS
#ifdef INJUGGLUCO
    .scanerrors={
        {"Skan xətası (%d)", "Yenidən cəhd edin"},
        {"Quraşdırma xətası", "?"},
        {"Məlumatın emalı xətası", "Yenidən cəhd edin"},
        {"Sensor aktivləşdirilir", ""},
        {"Sensorun istifadə müddəti qəti olaraq bitib", ""},
        {"Sensor hazır olacaq", "%d dəqiqədən sonra"},
        {"Sensor xətası (373)", "Sonra yenidən cəhd edin"},
        {"Yeni sensor işə salındı", "İstifadə etmək üçün yenidən skan edin"},
        {"", "Skan zamanı toxunmanı bloklayır"},
        {"", ""},
        {"Kitabxananın işə salınması xətası", "Paylaşılan kitabxanalar çatışmır?"},
        {"Sinfin işə salınması xətası", "Tədbir görün"},
        {"Əməliyyat çox uzanır", "Tətbiq dayandırılacaq"},
        {"Sensoru dəyişdirin (365)", "Sensorunuz işləmir. Onu çıxarın və yeni sensor başladın."},
        {"Sensoru dəyişdirin (368)", "Sensorunuz işləmir. Onu çıxarın və yeni sensor başladın."},
        {"", ""},
        {"Skan xətası", "Yenidən cəhd edin"}
    },
    .libre3scanerror={"FreeStyle Libre 3: skan xətası", "Yenidən cəhd edin"},
    .libre3wrongID={"Xəta: hesab ID-si yanlışdır?",
        R"(Sensor aktivləşdirilərkən istifadə olunan ID ilə eyni olmalıdır. LibreView-dan almaq üçün sol menyu → Tənzimləmələr → Məlumat mübadiləsi → LibreView → "Hesab ID-sini al" bölməsinə keçin.)"},
    .libre3scansuccess={"FreeStyle Libre 3 sensoru", "Qlükoza göstəriciləri artıq Juggluco-ya göndəriləcək"},
    .unknownNFC={"Tanınmayan NFC skanı xətası", "Yenidən cəhd edin"},
    .nolibre3={"FreeStyle Libre 3 sensoru", "Uyğun versiyanı https://www.juggluco.nl/download.html ünvanından endirin"},
    .libre3zeroID={"Xəta: hesab ID-si sıfırdır?",
        R"(Sıfırdan fərqli hesab ID-si təyin etmək üçün sol menyu → Tənzimləmələr → Məlumat mübadiləsi → LibreView → "Hesab ID-sini al" bölməsinə keçin.)"},
    .needsandroid8="Ən azı Android 8 tələb olunur"sv,
#ifndef WEAROS
    .advancedstart=R"(<h1>Dəyişdirilmiş qurğu</h1>
<p>Bu tətbiqdə istifadə edilən kitabxanalardan birində bəzi fayllar aşkar edildikdə proqramın çökməsinə səbəb olan səhv var.
Qurğunuzda həmin fayllardan bəziləri mövcuddur. Tətbiqdə bu səhvi yan keçən həll var, lakin həmin faylların başqa üsulla görünməz edilməsi daha məqsədəuyğundur.
Məsələn, Magisk müəyyən tətbiqlərdən root girişini gizlədə bilər (MagiskHide və ya DenyList); həmçinin Magisk-in adını dəyişmək mümkündür. Hər ikisi tələb olunur.
Sizin qurğunuzda problem yaradan fayl:)",
    .add_s=true,
    .shortinit=shortinit,
    .labels=labels,
#endif
#ifndef DONTTALK
    .checked="işarələnib",
    .unchecked="işarələnməyib",
    .Undetermined="Müəyyən edilməyib"sv,
    .FallingQuickly="Sürətlə azalır"sv,
    .Falling="Azalır"sv,
    .Stable="Yavaş dəyişir"sv,
    .Rising="Artır"sv,
    .RisingQuickly="Sürətlə artır"sv,
#endif
    .receivingpastvalues="Əvvəlki göstəricilər alınır"sv,
    .receivingdata="Məlumat alınır"sv,
    .unsupportedSibionics="Dəstəklənməyən Sibionics sensoru"sv,
    .waitingforconnection="Bağlantı gözlənilir"sv,
    .deleted="Silindi"sv,
    .nolocationpermission="Məkan icazəsi tələb olunur"sv,
    .nonearbydevicespermission="Yaxınlıqdakı qurğular icazəsi tələb olunur"sv,
#endif // INJUGGLUCO
    .summarygraph="Ümumi qrafik"sv,
    .logdays="Gündəlik jurnal"sv,
    .unhide="Göstər"sv
};


#include "logs.hpp"
addlang(az);
