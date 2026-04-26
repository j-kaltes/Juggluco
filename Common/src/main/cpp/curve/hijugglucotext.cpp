#include "jugglucotext.hpp"
#ifdef INJUGGLUCO
#ifndef WEAROS
// Note: elements of labels[] must be <= 11 bytes. Devanagari uses 3 bytes per
// character, so Hinglish (Roman-script Hindi) is used here for readability.
constexpr static std::string_view labels[]={"Carb",           // कार्बोहाइड्रेट (Carbohydrate)
"Dextro",                                                     // डेक्सट्रोज़ (Dextrose tablet)
"Tez Insuli",                                                 // तेज़ इंसुलिन (Fast Insulin)
"Lambi Insul",                                                // लंबी इंसुलिन (Long Insulin)
"Cycle",                                                      // साइकिल (Bike)
"Paidal",                                                     // पैदल (Walk)
"Khoon"};                                                     // खून (Blood)
// Shortcut names must also be <= 11 bytes. Hinglish used below.
constexpr static Shortcut_t  shortinit[]= { {"Roti",          // रोटी (Bread)
        .48},
        {"KishBun1",                                          // किशमिश बन 1 (Currantbun1)
        .56f},
        {"KishBun2",                                          // किशमिश बन 2 (Currantbun2)
        .595f},
        {"Angoor",                                            // अंगूर (Grapes)
        .165f},
        {"PhalDahi",                                          // फल दही (FruitYog)
        .058f},
        {"Chawal",                                            // चावल (Rice)
        .75f},
        {"Macaroni",                                          // मैकरोनी (Macaroni)
        .65f},
        {"Tamatar",                                           // टमाटर (Tomato)
        .03f},
        {"Mex Mix",                                           // मैक्सिकन मिक्स (Mexican mix)
        .078f},
        {"SantraRas",                                         // संतरा रस (Orange Juice)
        .109f},
        {"SportPwdr",                                         // स्पोर्ट पाउडर (Sport Powder)
        .873f},
        {"MixGajar",                                          // मिक्स (गाजर) (Mix Carrot)
        .07f},
        {"MixKhumb",                                          // मिक्स खुंभ (Mix mushroom)
        .07300000f}};
#endif
#endif //INJUGGLUCO

extern const jugglucotext hitext;
const jugglucotext hitext {
	.daylabel={"रवि", "सोम", "मंगल", "बुध", "गुरु", "शुक्र", "शनि"},
#ifdef INJUGGLUCO
.speakdaylabel={ "रविवार","सोमवार","मंगलवार","बुधवार","गुरुवार","शुक्रवार","शनिवार"},
	.monthlabel={
      "जन",
      "फर",
      "मार्च",
      "अप्रै",
      "मई"      ,
      "जून",
       "जुला",
       "अग",
       "सित",
      "अक्टू",
      "नव",
      "दिस"},

	.scanned="स्कैन किया",
	.readysecEnable="सेंसर %d मिनट में तैयार। स्ट्रीमिंग सक्षम करने के लिए पुनः स्कैन करें।",
	.readysec="सेंसर %d मिनट में तैयार।",
.networkproblem="मिरर से कोई ग्लूकोज नहीं",
.enablebluetooth="ब्लूटूथ सक्षम करें",
.useBluetoothOff="'ब्लूटूथ का उपयोग' बंद",
.noconnectionerror=": कोई कनेक्शन नहीं",
.stsensorerror=": सेंसर त्रुटि",
.streplacesensor=": सेंसर बदलें?",
.endedformat="%s ने काम करना बंद कर दिया। स्थिति=%d",
.notreadyformat="%s तैयार नहीं। स्थिति=%d",
#ifndef WEAROS
	.median="मध्यिका",
	.middle="मध्य",
#endif //WEAROS
	.history="इतिहास",
//	.historyinfo="Once per 15 minutes.\nRemembered on the sensor for 8 hours.\nScanning transfers them to this program.\nSensor: ", 
//	.history3info="Once per 5 minutes.\nRemembered on the sensor for 14 days.\nTransferred by Bluetooth to this program.\nSensor: ",
#ifndef WEAROS
	.sensorstarted= "शुरू किया:\t\t\t\t\t\t",
#else
	.sensorstarted= "शुरू किया:",
#endif
	.lastscanned="अंतिम स्कैन:",
#ifndef WEAROS
	.laststream="अंतिम स्ट्रीम:\t\t",
#else
	.laststream="अंतिम स्ट्रीम:",
#endif
	.sensorends="आधिकारिक समाप्ति: ",
	.sensorexpectedend="अपेक्षित समाप्ति: ",
#endif //INJUGGLUCO
#ifndef WEAROS

#ifdef INJUGGLUCO
	.newamount="नई मात्रा",
	.averageglucose="औसत ग्लूकोज: ",
	.duration="अवधि: %.1f दिन",
	.timeactive="%.1f%% समय सक्रिय",
	.nrmeasurement="मापों की संख्या: %d",
	.EstimatedA1C="अनुमानित A1C: %.1f%% (%d mmol/mol)",
	.GMI="ग्लूकोज प्रबंधन सूचक: %.1f%% (%d mmol/mol)",
	.SD="SD: %.2f",
	.glucose_variability="ग्लूकोज परिवर्तनशीलता: %.1f%%",
     .menustr0={
		"सिस्टम UI",
		"मेन्यू",
		"घड़ी",
		"सेंसर",
		"सेटिंग्स",

#if defined(SIBIONICS)
		"फोटो",
#else
		"बारे में",
#endif

		"बंद",
		"अलार्म रोकें"
		},
	.menustr1={
		"एक्सपोर्ट",
		"मिरर",
		hitext.newamount,
		"सूची", 
#else
       .statistics=
#endif //INJUGGLUCO
		"आंकड़े",
#ifdef INJUGGLUCO
		"बोलें",
		"फ्लोट"
		},
	.menustr2= {"अंतिम स्कैन",
	"स्कैन",
	"स्ट्रीम",
	"इतिहास",
	"मात्राएं",
	"भोजन",
	"डार्क मोड"},
	.menustr3= {hourminstr,
	"खोज",
	"तारीख",
	"पिछला दिन",
	"अगला दिन",
	"पिछला सप्ताह",
	"अगला सप्ताह"},
#endif //INJUGGLUCO
#else
 .amount="मात्रा",
 .menustr0= {
	"मिरर",
	"सेंसर",
	"डिस्प्ले",
    "सेटिंग्स",
	"अलार्म रोकें" },
.menustr2= {"तारीख",
hourminstr,
"पिछला दिन",
hitext.amount},
#endif //WEAROS
#ifdef INJUGGLUCO
	.scanerrors={
		{"स्कैन त्रुटि (%d)", "फिर से प्रयास करें"},
		{"इंस्टॉलेशन त्रुटि", "?"},
		{"डेटा प्रसंस्करण त्रुटि", "फिर से प्रयास करें"},
		{"सेंसर सक्रिय हो रहा है", ""},
		{"सेंसर निश्चित रूप से समाप्त हो गया है", ""},
		{"सेंसर तैयार होगा", "%d मिनट में"},
		{"सेंसर त्रुटि (373)", "बाद में पुनः प्रयास करें"},
		{"नया सेंसर प्रारंभ किया गया", "उपयोग करने के लिए पुनः स्कैन करें"},
		{"", "स्कैनिंग के दौरान स्पर्श अवरुद्ध करता है"},
		{"", ""},
		{"लाइब्रेरी आरंभीकरण त्रुटि", "क्या साझा लाइब्रेरी अनुपस्थित हैं?"},
		{"क्लास आरंभीकरण त्रुटि", "कुछ करें"},
		{"प्रक्रिया बहुत लंबी चलती है", "प्रोग्राम बंद कर रहा हूँ"},
		{"सेंसर बदलें (365)", "आपका सेंसर काम नहीं कर रहा। कृपया अपना सेंसर हटाएं और नया शुरू करें।"},
		{"सेंसर बदलें (368)", "आपका सेंसर काम नहीं कर रहा। कृपया अपना सेंसर हटाएं और नया शुरू करें।"},
		{"", ""},
		{"स्कैन त्रुटि", "फिर से प्रयास करें"}},

.libre3scanerror={"FreeStyle Libre 3, स्कैन त्रुटि", 
	"फिर से प्रयास करें"},
.libre3wrongID={"त्रुटि, गलत अकाउंट ID?",
	R"(यह वही होनी चाहिए जो सेंसर सक्रिय करते समय थी। Libreview से प्राप्त करने के लिए बाएं मेन्यू->सेटिंग्स->डेटा आदान-प्रदान->Libreview->"Account ID प्राप्त करें" का उपयोग करें।)"},
.libre3scansuccess= {"FreeStyle Libre 3 सेंसर", 
	"ग्लूकोज मान अब Juggluco द्वारा प्राप्त किए जाएंगे"},
.unknownNFC={"अज्ञात NFC स्कैन त्रुटि", 
	"फिर से प्रयास करें"},
.nolibre3={"FreeStyle Libre 3 सेंसर",
	"https://www.juggluco.nl/download.html से सही संस्करण डाउनलोड करें"},
.libre3zeroID={"त्रुटि, शून्य अकाउंट ID?",
	R"(गैर-शून्य अकाउंट ID सेट करने के लिए बाएं मेन्यू->सेटिंग्स->डेटा आदान-प्रदान->Libreview->"Account ID प्राप्त करें" का उपयोग करें।)"},
.needsandroid8="न्यूनतम Android 8 आवश्यक"sv,
#ifndef WEAROS
	.advancedstart= R"(<h1>संशोधित डिवाइस</h1>
<p>इस ऐप्लिकेशन द्वारा उपयोग की जाने वाली एक लाइब्रेरी में एक BUG है जो
कुछ फ़ाइलों का पता लगाने पर इसे क्रैश कर देता है। आपके डिवाइस में ऐसी कुछ
फ़ाइलें हैं। इस प्रोग्राम में इस BUG को दरकिनार करने की एक तरकीब है, पर
शायद बेहतर होगा कि इन फ़ाइलों को किसी और तरीके से अज्ञात बनाया जाए। उदाहरण
के लिए, Magisk में कुछ ऐप्लिकेशनों के लिए root छिपाने का विकल्प है
(Magiskhide या Denylist) और अपना नाम बदलने का भी, दोनों की आवश्यकता है।
आपके मामले में निम्नलिखित फ़ाइल से समस्या है)",
	.add_s=true,
.shortinit=shortinit,
.labels=labels,
#endif
#ifndef DONTTALK
.checked="चेक किया",
.unchecked="चेक नहीं किया",
.Undetermined=""sv,
.FallingQuickly="तेज़ी से गिर रहा"sv,
.Falling="गिर रहा"sv,
.Stable="धीरे-धीरे बदल रहा"sv,
.Rising="बढ़ रहा"sv,
.RisingQuickly="तेज़ी से बढ़ रहा"sv,
#endif
.receivingpastvalues="पुराने मान प्राप्त कर रहा"sv,
.receivingdata="डेटा प्राप्त कर रहा"sv,
.unsupportedSibionics="असमर्थित Sibionics सेंसर"sv,
.waitingforconnection="कनेक्शन की प्रतीक्षा"sv,
.deleted="हटाया गया"sv,
.nolocationpermission="स्थान की अनुमति आवश्यक"sv,
.nonearbydevicespermission="पास के डिवाइस की अनुमति आवश्यक"sv,
#endif //INJUGGLUCO
.summarygraph="सारांश ग्राफ़"sv
,.logdays="दैनिक लॉग"sv
,.unhide="दिखाएं"sv
		}


		;

#include "logs.hpp"
addlang(hi);
/*
void setusehi() {
LOGAR("setuse hi");
 usedtext= &hitext;
 }*/

//std::unordered_map<uint16_t,const jugglucotext*> langmap;
