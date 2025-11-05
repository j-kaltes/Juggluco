#include "jugglucotext.hpp"
#ifdef INJUGGLUCO
#ifndef WEAROS
constexpr static std::string_view trlabels[]={
"Karbonhidrt",
"Dextro",
"HizliEtkili",
"Uzun Etkili",
"Bisiklet",
"Yürüyüş",
"Kan Degeri"};
constexpr static Shortcut_t  trshortinit[]= { {"Ekmek",
        .48},
        {"Currantbun1",
        .56f},
        {"Currantbun2",
        .595f},
        {"Üzüm",
        .165f},
        {"Mey.Yoğ.",
        .058f},
        {"Pirinç",
        .75f},
        {"Makarna",
        .65f},
        {"Domates",
        .03f},
        {"Mexican mix",
        .078f},
        {"Port.Suyu",
        .109f},
        {"Prot.Tozu",
        .873f},
        {"Mix(Havuç)",
        .07f},
        {"Mix Mantar",
        .07300000f}};
#endif
#endif //INJUGGLUCO

extern jugglucotext trtext;
jugglucotext trtext {
        .daylabel={"Paz",
        "Pzt",
        "Sal",
        "Çrş",
        "Prş",
        "Cum",
        "Cts"},
#ifdef INJUGGLUCO
.speakdaylabel={ "Pazar","Pazartesi","Salı","Çarşamba","Perşembe","Cuma","Cumartesi"},
        .monthlabel={
      "Ock",
      "Şub",
      "Mar",
      "Nis",
      "May"      ,             
      "Haz",
       "Tem",
       "Ağs",
       "Eyl",
      "Eki",
      "Kas",
      "Ara"},

        .scanned="Tarandı",
        .readysecEnable="Sensör %d dakika içinde hazır. Akışı etkinleştirmek için tekrar tarayın.",
        .readysec="Sensör %d dakika içinde hazır.",
.networkproblem="Klondan glikoz değeri gelmiyor.",
.enablebluetooth="Bluetooth Etkin",
.useBluetoothOff="'Bluetooth Kullan' Kapat",
.noconnectionerror=": Bağlantı Yok",
.stsensorerror=": Sensör Hatası",
.streplacesensor=": Sensör değiştirilsin mi?",
.endedformat="%s sensörü artık çalışmıyor. Durum=%d",
.notreadyformat="%s sensörü hazır değil. Durum=%d",
#ifndef WEAROS
        .median="Median",
        .middle="Orta",
#endif
        .history="Geçmiş",
//      .historyinfo="15 dakikada bir.\nSensörde 8 saat boyunca saklanır.\nTarama ile bu programa aktarır.\nSensör: ", 
//      .history3info="5 dakikada bir.\nSensörde 14 gün boyunca saklanır.\nBluetooth ile bu programa aktarıldı.\nSensör: ",
        .sensorstarted= "Başlatıldı:",
        .lastscanned="Son Tarama:",
        .laststream="Son Akış:",
        .sensorends="Resmi olarak bitiş: ",
        .sensorexpectedend="Beklenen bitiş: ",
#endif //INJUGGLUCO
#ifndef WEAROS
#ifdef INJUGGLUCO
        .newamount="Yeni Miktar",
        .averageglucose="Ortalama Glikoz: ",
        .duration="Duration: %.1f gün",
        .timeactive="%.1f%% etkin zaman",
        .nrmeasurement="Ölçüm Sayısı: %d",
        .EstimatedA1C="Tahmini A1C: %.1f%% (%d mmol/mol)",
        .GMI="Glucose Management Indicator: %.1f%% (%d mmol/mol)",
        .SD="SD: %.2f",
        .glucose_variability="Glikoz Değişkenliği: %.1f%%",
     .menustr0={
                "Sistem Arayüzü        ",
                "Menüler",
                "Saat",
                "Sensör",
                "Ayarlar",

#if defined(SIBIONICS)
                "Fotoğraf",
#else
                "Hakkında",
#endif

                "Kapat",
                "Alarmı Durdur"
                },
        .menustr1={
                "Dışa Aktar",
                "Klon",
                trtext.newamount,
                "Liste", 
#else
       .statistics=
#endif //INJUGGLUCO
                "İstatistikler",
#ifdef INJUGGLUCO
                "Seslendirme",
                "Yüzen Glikoz       "
                },
	.menustr2= {"Calibrated",
        "Taramalar",
        "Akış",
        "Geçmiş",
        "Miktarlar",
        "Yemekler",
        "Koyu Mod        "},
        .menustr3= {hourminstr,
        "Ara",
        "Tarih",
        "Önceki Gün",
        "Sonraki Gün",
        "Önceki Hafta",
        "Sonraki Hafta"},
#endif //INJUGGLUCO
#else
 .amount="Miktar",
 .menustr0= {
        "        Klon",
        "Sensör",
        "Görüntü",
        "Ayarlar",
        "Alarmı Durdur" },
.menustr2= {"Tarih  ",
hourminstr,
"Önceki Gün                  ",
trtext.amount},
#endif

#ifdef INJUGGLUCO
        .scanerrors={
                {"Tarama Hatası (%d)", "Tekrar Deneyin"},
                {"Yükleme Hatası", "?"},
                {"Veri İşleme Hatası", "Tekrar Deneyin"},
                {"Sensör Etkinleştiriliyor", ""},
                {"Sensör Süresi Doldu", ""},
                {"Sensör Hazır", "%d dakika"},
                {"Sensör Hatası (373)", "Daha sonra Tekrar Deneyin"},
                {"Yeni Sensör başlatıldı", "Kullanmak için tekrar tarayın"},
                {"", "Tarama sırasında dokunmayı engeller"},
                {"", ""},
                {"Kütüphane başlatma hatası", "Paylaşılan kütüphaneler eksik mi?"},
                {"Sınıf başlatma hatası", "Bir şey yap"},
                {"İşlem çok uzun sürüyor", "Program durduruluyor."},
                {"Sensörü Değiştirin (365)", "Sensörünüz çalışmıyor. Lütfen Sensörünüzü çıkarın ve yenisini başlatın."},
                {" Sensörü Değiştirin (368)", "Sensörünüz çalışmıyor. Lütfen Sensörünüzü çıkarın ve yenisini başlatın."},
                {"", ""},
                {"Tarama Hatası", "Tekrar Deneyin"}},

.libre3scanerror={"FreeStyle Libre 3, Tarama Hatası", 
        "Tekrar Deneyin"},
.libre3wrongID={"Hata, Hesap kimliği yanlış mı?",
        R"(Sensör etkinleştirilirken kullanılan hesapla aynı olmalıdır. Hesap Kimliğinizi Libreview'den almak için Sol menü->Ayarlar->Veri değişimi->Libreview->"Hesap Kimliğini Al" seçeneğini kullanın.)"},
.libre3scansuccess= {"FreeStyle Libre 3 sensör", 
        "Glikoz değerleri artık Juggluco tarafından alınacak"},
.unknownNFC={"Tanınmayan NFC tarama Hatası", 
        "Tekrar Deneyin"},
.nolibre3={"FreeStyle Libre 3 sensor",
        "Doğru sürümü https://www.juggluco.nl/download.html adresinden indirin"},
.libre3zeroID={"Error, zero account ID?",
	R"(Use Left menu->Settings->Exchange data->Libreview->"Get Account ID" to set a non-zero account ID.)"},
.needsandroid8="En az Android 8 gerekir."sv,
#ifndef WEAROS
        .advancedstart= R"(<h1>Değiştirilmiş Cihaz</h1>
<p>Bu uygulama tarafından kullanılan kütüphanelerden birinde, belirli dosyaları algıladığında çökmesine neden olan bir HATA var. Cihazınız bu dosyalardan bazılarını içeriyor. Bu program bu BUG'ı aşmak için bir hack içeriyor, ancak
muhtemelen bu dosyaları başka bir şekilde tespit edilemez hale getirmek daha iyidir. Örneğin Magisk, belirli uygulamalar için (Magiskhide veya Denylist) kökü gizleme ve kendi adını değiştirme seçeneğine sahiptir, her ikisi de gereklidir. Sizin durumunuzda aşağıdaki dosyayla ilgili sorunlar var.)",
        .add_s=true,
.shortinit=trshortinit,
.labels=trlabels,
#endif
#ifndef DONTTALK
.checked="Kontrol edildi",
.unchecked="Kontrol edilmedi",
.Undetermined=""sv,
.FallingQuickly="Hızlı Düşüyor"sv,
.Falling="Düşüyor"sv,
.Stable="Sabit"sv,
.Rising="Yükseliyor"sv,
.RisingQuickly="Hızlı Yükseliyor"sv,
#endif
.receivingpastvalues="Eski Veriler Alınıyor"sv,
.receivingdata="Veri Alınıyor"sv,
.unsupportedSibionics="Desteklenmeyen Sibionics Sensörü"sv,
.waitingforconnection="Bağlantı bekleniyor"sv,
.deleted="Silindi"sv,
.nolocationpermission="Konum izni gerekiyor"sv,
.nonearbydevicespermission="Yakındaki Cihazlar izni gerekiyor"sv
#endif //INJUGGLUCO
,.summarygraph="Özet Grafiği"sv
,.logdays="Günler"sv
,.unhide="Unhide"sv
                }


                ;

addlang(tr);

