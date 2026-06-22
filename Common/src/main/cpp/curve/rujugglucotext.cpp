#include "jugglucotext.hpp"
#ifdef INJUGGLUCO
#ifndef WEAROS
constexpr static std::string_view rulabels[]={
"Угл.",
"Сахар",
"Болус",
"Базал",
"Вело",
"Шаги",
"Кровь"};




constexpr static Shortcut_t  rushortinit[]= { {"Хлеб",
        .48},
        {"Изюм1",
        .56f},
        {"Изюм2",
        .595f},
        {"Виног",
        .165f},
        {"ЙогФр",
        .058f},
        {"Рис",
        .75f},
        {"Макар",
        .65f},
        {"Томат",
        .03f},
        {"Мекс",
        .078f},
        {"АпСок",
        .109f},
        {"Спорт",
        .873f},
        {"Моркв",
        .07f},
        {"Грибы",
        .07300000f}};
#endif

#endif //INJUGGLUCO
jugglucotext rutext {
	.daylabel={"Вс",
	"Пн",
	"Вт",
	"Ср",
	"Чт",
	"Пт",
	"Сб"},
#ifdef INJUGGLUCO
.speakdaylabel={ "Воскресенье","Понедельник","Вторник","Среда","Четверг","Пятница","Суббота"},
	.monthlabel={
      "Янв",
      "Фев",
      "Мар",
      "Апр",
      "Май"      ,             
      "Июн",
       "Июл",
       "Авг",
       "Сен",
      "Окт",
      "Ноя",
      "Дек"},

	.scanned="Скан",
	.readysecEnable="Датчик будет готов через %d минут. Отсканируйте еще раз, чтобы включить поток.",
	.readysec="Датчик будет готов через %d минут.",
.networkproblem="Нет глюкозы от зеркала",
.enablebluetooth="Включить Bluetooth",
.useBluetoothOff="'Использовать Bluetooth' выключено",
.noconnectionerror=": Нет соединения",
.stsensorerror=": Ошибка датчика",
.streplacesensor=": Заменить датчик?",
.endedformat="%s перестал работать. Состояние=%d",
.notreadyformat="%s не готов. Состояние=%d",
#ifndef WEAROS
	.median="Медиана",
	.middle="Среднее",
#endif
	.history="История",
//	.historyinfo="Once per 15 minutes.\nRemembered on the sensor for 8 hours.\nScanning transfers them to this program.\nSensor: ", 
//	.history3info="Once per 5 minutes.\nRemembered on the sensor for 14 days.\nTransferred by Bluetooth to this program.\nSensor: ",
#ifndef WEAROS
	.sensorstarted= "Начало:\t\t\t\t\t\t",
#else
	.sensorstarted= "Начало:",
#endif
	.lastscanned="Посл. скан:",
#ifndef WEAROS
	.laststream="Посл. поток:\t",
#else
	.laststream="Посл. поток:",
#endif
	.sensorends="Официальное окончание: ",
    .sensorexpectedend="Ожидаемое окончание: ",
#endif //INJUGGLUCO
#ifndef WEAROS
#ifdef INJUGGLUCO
	.newamount="Новая запись",
	.averageglucose="Средняя глюкоза: ",
	.duration="Продолжительность: %.1f дн.",
	.timeactive="%.1f%% времени активно",
	.nrmeasurement="Число измерений: %d",
	.EstimatedA1C="Расчетный A1C: %.1f%% (%d ммоль/моль)",
	.GMI="Индикатор управления глюкозой: %.1f%% (%d ммоль/моль)",
	.SD="SD: %.2f",
	.glucose_variability="Вариабельность глюкозы: %.1f%%",
     .menustr0={
		"System UI",
		"Меню",
		"Часы",
		"Датчик",
		"Настройки",
#if defined(SIBIONICS)
		"Фото",
#else
		"О программе",
#endif
		"Закрыть",
		"Стоп тревога"
		},
	.menustr1={
		"Экспорт",
		"Зеркало",
		 rutext.newamount,
		"Список",
#else
       .statistics=
#endif //INJUGGLUCO
		"Статистика",
#ifdef INJUGGLUCO
		"Озвучка",
		"Плав. глюк."
		},
	.menustr2= {"Скан",
	"Сканы",
	"Поток",
	"История",
	"Записи",
	"Еда",
	"Темная тема"},
	.menustr3= {hourminstr,
	"Поиск",
	"Дата",
	"День назад",
	"День вперед",
	"Неделя назад",
	"Неделя вперед"},
#endif //INJUGGLUCO
#else
 .amount="Запись",
 .menustr0= {
	"        Зеркало",
	"Датчик",
	"Дисплей",
    "Настройки",
	"Стоп тревога" },
.menustr2= {"Дата",
hourminstr,
"День назад",
rutext.amount},
#endif

#ifdef INJUGGLUCO
	.scanerrors={
		{"Ошибка скана (%d)", "Попробуйте еще"},
		{"Ошибка установки", "?"},
		{"Ошибка обработки данных", "Попробуйте еще"},
		{"Активация датчика", ""},
		{"Датчик точно завершился", ""},
		{"Датчик будет готов через", "%d мин"},
		{"Ошибка датчика (373)", "Попробуйте позже"},
		{"Новый датчик инициализирован", "Отсканируйте еще раз, чтобы использовать"},
		{"", "Блокирует касания во время сканирования"},
		{"", ""},
		{"Ошибка инициализации библиотеки", "Отсутствуют общие библиотеки?"},
		{"Ошибка инициализации класса", "Сделайте что-нибудь"},
		{"Процедура длится слишком долго", "Программа будет закрыта"},
		{"Заменить датчик (365)", "Ваш датчик не работает. Снимите его и начните новый."},
		{"Заменить датчик (368)", "Ваш датчик не работает. Снимите его и начните новый."},
		{"", ""},
		{"Ошибка скана", "Попробуйте еще"}},

.libre3scanerror={"FreeStyle Libre 3, ошибка скана", 
	"Попробуйте еще"},
.libre3wrongID={"Ошибка: неверный ID учетной записи?",
	R"(Он должен совпадать с тем, который использовался при активации датчика. Чтобы получить его из LibreView, откройте левое меню→Настройки→Обмен данными→LibreView→"Получить Account ID")"},
.libre3scansuccess= {"Датчик FreeStyle Libre 3", 
	"Juggluco теперь будет получать значения глюкозы"},
.unknownNFC={"Нераспознанная ошибка сканирования NFC", 
	"Попробуйте еще"},
.nolibre3={"Датчик FreeStyle Libre 3",
	"Загрузите правильную версию с https://www.juggluco.nl/download.html"},
.libre3zeroID={"Ошибка: нулевой ID учетной записи?",
	R"(Откройте левое меню→Настройки→Обмен данными→LibreView→"Получить Account ID", чтобы установить ненулевой Account ID.)"},
.needsandroid8="Требуется как минимум Android 8"sv,
#ifndef WEAROS
	.advancedstart= R"(<h1>Модифицированное устройство</h1>
<p>В одной из библиотек, используемых этим приложением, есть ошибка, из-за которой
она аварийно завершается при обнаружении определенных файлов. На вашем устройстве
есть некоторые из этих файлов. В программе есть обходной прием для этой ошибки, но,
вероятно, лучше сделать эти файлы необнаружимыми другим способом. Например, в Magisk
есть возможность скрыть root для отдельных приложений (MagiskHide или DenyList) и
изменить собственное имя; нужны оба действия. В вашем случае проблема связана со
следующим файлом)",
	.add_s=true,
.shortinit=rushortinit,
.labels=rulabels,
#endif
#ifndef DONTTALK
.checked="Отмечено",
.unchecked="Не отмечено",
.Undetermined=""sv,
.FallingQuickly="Быстро падает"sv,
.Falling="Падает"sv,
.Stable="Медленно меняется"sv,
.Rising="Растет"sv,
.RisingQuickly="Быстро растет"sv,
#endif
.receivingpastvalues="Получение старых значений"sv,
.receivingdata="Получение данных"sv,
.unsupportedSibionics="Датчик Sibionics не поддерживается"sv,
.waitingforconnection="Ожидание соединения"sv,
.deleted="Удалено"sv,
.nolocationpermission="Требуется разрешение на местоположение"sv,
.nonearbydevicespermission="Требуется разрешение на близлежащие устройства"sv

#endif //INJUGGLUCO
 ,.summarygraph="Сводный график"sv


,.logdays="Дневной журнал"sv 
,.unhide="Показать"sv

		}


		;

#include "logs.hpp"
extern void setuseru();
addlang(ru);
void setuseru() {
LOGAR("Использование ru");
 usedtext= &rutext;
 }
