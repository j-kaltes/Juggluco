#include "jugglucotext.hpp"
#ifdef INJUGGLUCO
#define KEEP(x) x
#ifndef WEAROS
constexpr static std::string_view labels[]={
 RTL("کرب"), RTL("دکس"), RTL("سری"),
 RTL("پای"), RTL("دوچ"), RTL("پیا"), RTL("خون")};
constexpr static Shortcut_t shortinit[]={
 {KEEP("نان"), .48},
 {KEEP("کشم۱"), .56f},
 {KEEP("کشم۲"), .595f},
 {KEEP("انگور"), .165f},
 {KEEP("ماست"), .058f},
 {KEEP("برنج"), .75f},
 {KEEP("ماکار"), .65f},
 {KEEP("گوجه"), .03f},
 {KEEP("مخلوط"), .078f},
 {KEEP("آب‌پو"), .109f},
 {KEEP("پودر"), .873f},
 {KEEP("هویج"), .07f},
 {KEEP("قارچ"), .07300000f}};
#endif
#endif // INJUGGLUCO

extern const jugglucotext fatext;
const jugglucotext fatext {
 .daylabel={RTL("یکش"), RTL("دوش"), RTL("سه‌ش"), RTL("چهار"), RTL("پنج"), RTL("جمع"), RTL("شنبه")},
#ifdef INJUGGLUCO
 .speakdaylabel={KEEP("یکشنبه"), KEEP("دوشنبه"), KEEP("سه‌شنبه"), KEEP("چهارشنبه"), KEEP("پنجشنبه"), KEEP("جمعه"), KEEP("شنبه")},
 .monthlabel={RTL("ژان"), RTL("فور"), RTL("مار"), RTL("آور"), RTL("مه"), RTL("ژوئ"), RTL("ژوئی"), RTL("اوت"), RTL("سپت"), RTL("اکت"), RTL("نوا"), RTL("دسا")},
 .scanned=RTL("اسکن شد"),
 .readysecEnable=RTLFMT("حسگر تا %d دقیقه دیگر آماده می‌شود. برای فعال‌کردن دریافت پیوسته، دوباره اسکن کنید."),
 .readysec=RTLFMT("حسگر تا %d دقیقه دیگر آماده می‌شود."),
 .networkproblem=RTL("مقدار گلوکز از دستگاه همگام دریافت نشد"),
 .enablebluetooth=RTL("بلوتوث را روشن کنید"),
 .useBluetoothOff=RTL("«استفاده از بلوتوث» خاموش است"),
 .noconnectionerror=RTL(": اتصال برقرار نیست"),
 .stsensorerror=RTL(": خطای حسگر"),
 .streplacesensor=RTL(": حسگر تعویض شود؟"),
 .endedformat=RTLFMT("%s از کار افتاده است. وضعیت=%d"),
 .notreadyformat=RTLFMT("%s آماده نیست. وضعیت=%d"),
#ifndef WEAROS
 .median=RTL("میانه"),
 .middle=RTL("وسط"),
#endif
 .history=RTL("تاریخچه"),
#ifndef WEAROS
 .sensorstarted=RTL("شروع:						"),
#else
 .sensorstarted=RTL("شروع:"),
#endif
 .lastscanned=RTL("آخرین اسکن:"),
#ifndef WEAROS
 .laststream=RTL("آخرین دریافت پیوسته:		"),
#else
 .laststream=RTL("آخرین دریافت پیوسته:"),
#endif
 .sensorends=RTL("پایان رسمی: "),
 .sensorexpectedend=RTL("پایان مورد انتظار: "),
#endif // INJUGGLUCO
#ifndef WEAROS
#ifdef INJUGGLUCO
 .newamount=RTL("مقدار جدید"),
 .averageglucose=RTL("میانگین گلوکز: "),
 .duration=RTLFMT("مدت: %.1f روز"),
 .timeactive=RTLFMT("در %.1f%% از زمان فعال بوده است"),
 .nrmeasurement=RTLFMT("تعداد اندازه‌گیری‌ها: %d"),
 .EstimatedA1C=RTLFMT("uGMI ≈ HbA1c تخمینی: %.1f%% (%d mmol/mol)"),
 .GMI=RTLFMT("شاخص مدیریت گلوکز (GMI): %.1f%% (%d mmol/mol)"),
 .SD=RTLFMT("انحراف معیار: %.2f"),
 .glucose_variability=RTLFMT("تغییرپذیری گلوکز: %.1f%%"),
 .menustr0={"ChatGPT", RTL("منوها"), RTL("ساعت"), RTL("حسگر"), RTL("تنظیمات"),
#if defined(SIBIONICS)
 RTL("عکس"),
#else
 RTL("درباره"),
#endif
 RTL("بستن"), RTL("قطع هشدار")},
 .menustr1={RTL("خروجی"), RTL("آینه‌سازی"), fatext.newamount, RTL("فهرست"),
#else
 .statistics=
#endif
 RTL("آمار"),
#ifdef INJUGGLUCO
 RTL("گفتار"), RTL("نمایش شناور")},
 .menustr2={RTL("رابط سیستم"), RTL("اسکن‌ها"), RTL("داده‌های پیوسته"), RTL("تاریخچه"),
 RTL("مقدارها"), RTL("وعده‌های غذایی"), RTL("حالت تیره")},
 .menustr3={hourminstr, RTL("جست‌وجو"), RTL("تاریخ"), RTL("روز قبل"),
 RTL("روز بعد"), RTL("هفتهٔ قبل"), RTL("هفتهٔ بعد")},
#endif // INJUGGLUCO
#else
 .amount=RTL("مقدار"),
 .menustr0={RTL("آینه‌سازی"), RTL("حسگر"), RTL("نمایش"), RTL("تنظیمات"), RTL("قطع هشدار")},
 .menustr2={RTL("تاریخ"), hourminstr, RTL("روز قبل"), fatext.amount},
#endif // WEAROS
#ifdef INJUGGLUCO
 .scanerrors={
  {RTLFMT("خطای اسکن (%d)"), RTL("دوباره تلاش کنید")},
  {RTL("خطای نصب"), "?"},
  {RTL("خطا در پردازش داده‌ها"), RTL("دوباره تلاش کنید")},
  {RTL("در حال فعال‌سازی حسگر"), ""},
  {RTL("عمر حسگر قطعاً به پایان رسیده است"), ""},
  {RTL("حسگر آماده می‌شود تا"), RTLFMT("%d دقیقه")},
  {RTL("خطای حسگر (373)"), RTL("کمی بعد دوباره تلاش کنید")},
  {RTL("حسگر جدید مقداردهی اولیه شد"), RTL("برای استفاده دوباره اسکن کنید")},
  {"", RTL("هنگام اسکن، لمس صفحه مسدود می‌شود")},
  {"", ""},
  {RTL("خطا در مقداردهی اولیهٔ کتابخانه"), RTL("آیا کتابخانه‌های اشتراکی وجود ندارند؟")},
  {RTL("خطا در مقداردهی اولیهٔ کلاس"), RTL("اقدامی انجام دهید")},
  {RTL("عملیات بیش از حد طول کشیده است"), RTL("برنامه بسته می‌شود")},
  {RTL("تعویض حسگر (365)"), RTL("حسگر کار نمی‌کند. آن را بردارید و حسگر جدیدی را شروع کنید.")},
  {RTL("تعویض حسگر (368)"), RTL("حسگر کار نمی‌کند. آن را بردارید و حسگر جدیدی را شروع کنید.")},
  {"", ""},
  {RTL("خطای اسکن"), RTL("دوباره تلاش کنید")}},
 .libre3scanerror={RTL("FreeStyle Libre 3: خطای اسکن"), RTL("دوباره تلاش کنید")},
 .libre3wrongID={RTL("خطا: شناسهٔ حساب نادرست است؟"),
  RTL(u8R"(باید همان شناسه‌ای باشد که هنگام فعال‌سازی حسگر استفاده شده است. برای دریافت آن از LibreView، به منوی چپ ← تنظیمات ← تبادل داده ← LibreView ← «دریافت شناسهٔ حساب» بروید.)")},
 .libre3scansuccess={RTL("حسگر FreeStyle Libre 3"), RTL("اکنون Juggluco مقدارهای گلوکز را دریافت می‌کند")},
 .unknownNFC={RTL("خطا در اسکن NFC ناشناخته"), RTL("دوباره تلاش کنید")},
 .nolibre3={RTL("حسگر FreeStyle Libre 3"), RTL("نسخهٔ مناسب را از https://www.juggluco.nl/download.html دریافت کنید")},
 .libre3zeroID={RTL("خطا: شناسهٔ حساب صفر است؟"),
  RTL(u8R"(برای تعیین شناسهٔ حساب غیرصفر، به منوی چپ ← تنظیمات ← تبادل داده ← LibreView ← «دریافت شناسهٔ حساب» بروید.)")},
 .needsandroid8=RTL("حداقل Android 8 لازم است"),
#ifndef WEAROS
 .advancedstart=R"(<h1>دستگاه تغییریافته</h1><p>یکی از کتابخانه‌های مورد استفادهٔ این برنامه اشکالی دارد که با شناسایی بعضی فایل‌ها موجب بسته‌شدن برنامه می‌شود. چنین فایل‌هایی در دستگاه شما وجود دارند. برنامه راهکاری برای دورزدن این اشکال دارد، ولی بهتر است این فایل‌ها به روش دیگری از دید کتابخانه پنهان شوند. برای نمونه، Magisk می‌تواند دسترسی روت را برای برنامه‌های خاص مخفی کند (MagiskHide یا DenyList) و نام خودش را نیز تغییر دهد؛ هر دو لازم‌اند. در دستگاه شما فایل زیر مشکل‌ساز است:)" ,
 .add_s=true,
 .shortinit=shortinit,
 .labels=labels,
#endif
#ifndef DONTTALK
 .checked=KEEP("انتخاب‌شده"),
 .unchecked=KEEP("انتخاب‌نشده"),
 .Undetermined=""sv,
 .FallingQuickly=KEEP("به‌سرعت در حال کاهش"),
 .Falling=KEEP("در حال کاهش"),
 .Stable=KEEP("تغییر آهسته"),
 .Rising=KEEP("در حال افزایش"),
 .RisingQuickly=KEEP("به‌سرعت در حال افزایش"),
#endif
 .receivingpastvalues=RTL("در حال دریافت مقدارهای گذشته"),
 .receivingdata=RTL("در حال دریافت داده‌ها"),
 .unsupportedSibionics=RTL("حسگر Sibionics پشتیبانی نمی‌شود"),
 .waitingforconnection=RTL("در انتظار اتصال"),
 .deleted=RTL("حذف شد"),
 .nolocationpermission=RTL("مجوز مکان لازم است"),
 .nonearbydevicespermission=RTL("مجوز دستگاه‌های نزدیک لازم است"),
#endif // INJUGGLUCO
 .summarygraph=RTL("نمودار خلاصه"),
 .logdays=RTL("دفتر ثبت روزانه"),
 .unhide=RTL("نمایش"),
 .spokenmonthlabelptr=&jugglucotext::spokenmonthlabelrtl
};

language language_fa("fa", &fatext);
#include "logs.hpp"
