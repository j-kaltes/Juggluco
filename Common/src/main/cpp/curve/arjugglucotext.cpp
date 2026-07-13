#include "jugglucotext.hpp"
#ifdef INJUGGLUCO
#define KEEP(x) x
#ifndef WEAROS
constexpr static std::string_view labels[]={RTL(u8"كرب"),
RTL(u8"دكس"),
RTL(u8"سري"),
RTL(u8"طوي"),
RTL(u8"درج"),
RTL(u8"مشي"),
RTL(u8"دم")};
constexpr static Shortcut_t  shortinit[]= { {u8"خبز",
        .48},
        {KEEP(u8"زبي1"),
        .56f},
        {KEEP(u8"زبي2"),
        .595f},
        {KEEP(u8"عنب"),
        .165f},
        {KEEP(u8"يوغ"),
        .058f},
        {KEEP(u8"أرز"),
        .75f},
        {KEEP(u8"مكر"),
        .65f},
        {KEEP(u8"طمط"),
        .03f},
        {KEEP(u8"خلط"),
        .078f},
        {KEEP(u8"عصي"),
        .109f},
        {KEEP(u8"بود"),
        .873f},
        {KEEP(u8"جزر"),
        .07f},
        {KEEP(u8"فطر"),
        .07300000f}};
#endif
#endif //INJUGGLUCO

extern const jugglucotext artext;
const jugglucotext artext {
	.daylabel={RTL(u8"أحد"), RTL(u8"اثن"), RTL(u8"ثلا"), RTL(u8"أرب"), RTL(u8"خمي"), RTL(u8"جمع"), RTL(u8"سبت")},
#ifdef INJUGGLUCO
.speakdaylabel={ KEEP(u8"الأحد"),KEEP(u8"الاثنين"),KEEP(u8"الثلاثا"),KEEP(u8"الأربعا"),KEEP(u8"الخميس"),KEEP(u8"الجمعة"),KEEP(u8"السبت")},
	.monthlabel={
      RTL(u8"ينا"),
      RTL(u8"فبر"),
      RTL(u8"مار"),
      RTL(u8"أبر"),
      RTL(u8"ماي")      ,             
      RTL(u8"يون"),
       RTL(u8"يول"),
       RTL(u8"أغس"),
       RTL(u8"سبت"),
      RTL(u8"أكت"),
      RTL(u8"نوف"),
      RTL(u8"ديس")},

	.scanned=RTL(u8"تم المسح"),
	.readysecEnable=RTLFMT(u8"سيصبح المستشعر جاهزًا خلال %d دقيقة. امسحه مرة أخرى لتفعيل البث."),
	.readysec=RTLFMT(u8"سيصبح المستشعر جاهزًا خلال %d دقيقة."),
.networkproblem=RTL(u8"لا توجد قراءات غلوكوز من الاستنساخ"),
.enablebluetooth=RTL(u8"فعّل البلوتوث"),
.useBluetoothOff=RTL(u8"تم إيقاف \"استخدام البلوتوث\""),
.noconnectionerror=RTL(u8": لا يوجد اتصال"),
.stsensorerror=RTL(u8": خطأ في المستشعر"),
.streplacesensor=RTL(u8": استبدال المستشعر؟"),
.endedformat=RTLFMT(u8"%s توقف عن العمل. الحالة=%d"),
.notreadyformat=RTLFMT(u8"%s غير جاهز. الحالة=%d"),
#ifndef WEAROS
	.median=RTL(u8"الوسيط"),
	.middle=RTL(u8"الوسط"),
#endif //WEAROS
	.history=RTL(u8"السجل"),
#ifndef WEAROS
	.sensorstarted= RTL(u8"بدأ:\t\t\t\t\t\t"),
#else
	.sensorstarted= RTL(u8"بدأ:"),
#endif
	.lastscanned=RTL(u8"آخر مسح:"),
#ifndef WEAROS
	.laststream=RTL(u8"آخر بث:\t\t"),
#else
	.laststream=RTL(u8"آخر بث:"),
#endif
	.sensorends=RTL(u8"ينتهي رسميًا: "),
	.sensorexpectedend=RTL(u8"النهاية المتوقعة: "),
#endif //INJUGGLUCO
#ifndef WEAROS

#ifdef INJUGGLUCO
	.newamount=RTL(u8"كمية جديدة"),
	.averageglucose=RTL(u8"متوسط الغلوكوز: "),
	.duration=RTLFMT(u8"المدة: %.1f يوم"),
	.timeactive=RTLFMT(u8"%.1f%% من الوقت كان نشطًا"),
	.nrmeasurement=RTLFMT(u8"عدد القياسات: %d"),



//	.EstimatedA1C=RTLFMT(u8"A1C التقديري: %.1f%% (%d mmol/mol)"),
//    .EstimatedA1C=RTLFMT(u8"التقديري A1C: %.1f%% (%d mmol/mol)"),
    .EstimatedA1C = RTLFMT(u8"%.1f%% (%d mmol/mol) :A1C التقديري"),
//	.GMI=RTL(u8"%.1f%% (%d mmol/mol) :مؤشر إدارة الغلوكوز"),
	.GMI=RTLFMT(u8"مؤشر إدارة الغلوكوز: %.1f%% (%d mmol/mol)"),
	.SD=RTLFMT(u8"الانحراف المعياري: %.2f"),
	.glucose_variability=RTLFMT(u8"تذبذب الغلوكوز: %.1f%%"),
     .menustr0={
		RTL(u8"واجهة النظام"),
		RTL(u8"القوائم"),
		RTL(u8"الساعة"),
		RTL(u8"المستشعر"),
		RTL(u8"الإعدادات"),

#if defined(SIBIONICS)
		RTL(u8"صورة"),
#else
		RTL(u8"حول"),
#endif

		RTL(u8"إغلاق"),
		RTL(u8"إيقاف التنبيه")
		},
	.menustr1={
		RTL(u8"تصدير"),
		RTL(u8"استنساخ"),
		artext.newamount,
		RTL(u8"قائمة"), 
#else
       .statistics=
#endif //INJUGGLUCO
		RTL(u8"الإحصاءات"),
#ifdef INJUGGLUCO
		RTL(u8"نطق"),
		RTL(u8"عائم")
		},

	.menustr2= {RTL(u8"آخر قراءة"),
	RTL(u8"القراءات"),

	RTL(u8"البث"),
	RTL(u8"السجل"),
	RTL(u8"الكميات"),
	RTL(u8"الوجبات"),
	RTL(u8"الوضع الداكن")},
	.menustr3= {hourminstr,
	RTL(u8"بحث"),
	RTL(u8"التاريخ"),
	RTL(u8"اليوم السابق"),
	RTL(u8"اليوم التالي"),
	RTL(u8"الأسبوع السابق"),
	RTL(u8"الأسبوع التالي")},
#endif //INJUGGLUCO
#else
 .amount=RTL(u8"كمية"),
 .menustr0= {
	RTL(u8"استنساخ"),
	RTL(u8"المستشعر"),
	RTL(u8"العرض"),
    RTL(u8"الإعدادات"),
	RTL(u8"إيقاف التنبيه") },
.menustr2= {RTL(u8"التاريخ"),
hourminstr,
RTL(u8"اليوم السابق"),
artext.amount},
#endif //WEAROS
#ifdef INJUGGLUCO
	.scanerrors={
		{RTLFMT(u8"خطأ في المسح (%d)"), RTL(u8"حاول مرة أخرى")},
		{RTL(u8"خطأ في التثبيت"), "?"},
		{RTL(u8"خطأ في معالجة البيانات"), RTL(u8"حاول مرة أخرى")},
		{RTL(u8"جارٍ تفعيل المستشعر"), ""},
		{RTL(u8"انتهت صلاحية المستشعر نهائيًا"), ""},
		{RTLFMT(u8"سيصبح المستشعر جاهزًا خلال"), RTL(u8"%d دقيقة")},
		{RTL(u8"خطأ في المستشعر (373)"), RTL(u8"حاول مرة أخرى لاحقًا")},
		{RTL(u8"تم تجهيز مستشعر جديد"), RTL(u8"امسحه مرة أخرى لاستخدامه")},
		{"", RTL(u8"يحظر اللمس أثناء المسح")},
		{"", ""},
		{RTL(u8"خطأ في تهيئة المكتبة"), RTL(u8"هل المكتبات المشتركة مفقودة؟")},
		{RTL(u8"خطأ في تهيئة الصنف"), RTL(u8"افعل شيئًا")},
		{RTL(u8"الإجراء يستغرق وقتًا طويلًا"), RTL(u8"سأغلق البرنامج")},
		{RTL(u8"استبدل المستشعر (365)"), RTL(u8"المستشعر لا يعمل. يُرجى إزالة المستشعر والبدء بآخر جديد.")},
		{RTL(u8"استبدل المستشعر (368)"), RTL(u8"المستشعر لا يعمل. يُرجى إزالة المستشعر والبدء بآخر جديد.")},
		{"", ""},
		{RTL(u8"خطأ في المسح"), RTL(u8"حاول مرة أخرى")}},

.libre3scanerror={RTL(u8"FreeStyle Libre 3، خطأ في المسح"), 
	RTL(u8"حاول مرة أخرى")},
.libre3wrongID={RTL(u8"خطأ، هل معرّف الحساب غير صحيح؟"),
	RTL(u8R"(يجب أن يكون مطابقًا للمعرّف المستخدم عند تفعيل المستشعر. استخدم القائمة اليسرى ← الإعدادات ← تبادل البيانات ← Libreview ← \"الحصول على معرّف الحساب\" لاسترجاعه من Libreview.)")},
.libre3scansuccess= {RTL(u8"مستشعر FreeStyle Libre 3"), 
	RTL(u8"سيستقبل Juggluco الآن قيم الغلوكوز")},
.unknownNFC={RTL(u8"خطأ مسح NFC غير معروف"), 
	RTL(u8"حاول مرة أخرى")},
.nolibre3={RTL(u8"مستشعر FreeStyle Libre 3"),
	RTL(u8"نزّل الإصدار الصحيح من https://www.juggluco.nl/download.html")},
.libre3zeroID={RTL(u8"خطأ، هل معرّف الحساب صفر؟"),
	RTL(u8R"(استخدم القائمة اليسرى ← الإعدادات ← تبادل البيانات ← Libreview ← \"الحصول على معرّف الحساب\" لضبط معرّف حساب غير صفري.)")},
.needsandroid8=RTL(u8"يلزم Android 8 على الأقل"),
#ifndef WEAROS
	.advancedstart= R"(<h1>جهاز معدل</h1>
<p>إحدى المكتبات المستخدمة في هذا التطبيق تحتوي على خلل يؤدي إلى تعطله إذا اكتشفت ملفات معينة. يحتوي جهازك على بعض هذه الملفات. يتضمن هذا البرنامج حيلة لتجاوز هذا الخلل، لكن من الأفضل غالبًا جعل هذه الملفات غير قابلة للاكتشاف بطريقة أخرى. على سبيل المثال، يتيح Magisk إخفاء صلاحيات الروت عن بعض التطبيقات (MagiskHide أو DenyList) وتغيير اسمه أيضًا، وكلا الأمرين مطلوبان. في حالتك توجد مشكلة مع الملف التالي)</p>)",
	.add_s=true,
.shortinit=shortinit,
.labels=labels,
#endif
#ifndef DONTTALK
.checked=KEEP("محدَّد"),
.unchecked=KEEP("غير محدَّد"),
.Undetermined=""sv,
.FallingQuickly=KEEP("ينخفض بسرعة"),
.Falling=KEEP("ينخفض"),
.Stable=KEEP("يتغير ببطء"),
.Rising=KEEP("يرتفع"),
.RisingQuickly=KEEP("يرتفع بسرعة"),
#endif
.receivingpastvalues=RTL(u8"جارٍ استقبال القيم القديمة"),
.receivingdata=RTL(u8"جارٍ استقبال البيانات"),
.unsupportedSibionics=RTL(u8"مستشعر Sibionics غير مدعوم"),
.waitingforconnection=RTL(u8"في انتظار الاتصال"),
.deleted=RTL(u8"تم الحذف"),
.nolocationpermission=RTL(u8"يلزم إذن الموقع"),
.nonearbydevicespermission=RTL(u8"يلزم إذن الأجهزة القريبة"),
#endif //INJUGGLUCO
.summarygraph=RTL(u8"الرسم البياني الملخص")
,.logdays=RTL(u8"السجل اليومي")
,.unhide=RTL(u8"إظهار"),

.spokenmonthlabelptr = &jugglucotext::spokenmonthlabelrtl
		}


		;

language language_ar("ar",&artext);
#include "logs.hpp"
