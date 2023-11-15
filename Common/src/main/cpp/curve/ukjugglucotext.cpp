/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2 and 3 sensors.                         */
/*                                                                                   */
/*      Copyright (C) 2021 Jaap Korthals Altes <jaapkorthalsaltes@gmail.com>         */
/*                                                                                   */
/*      Juggluco is free software: you can redistribute it and/or modify             */
/*      it under the terms of the GNU General Public License as published            */
/*      by the Free Software Foundation, either version 3 of the License, or         */
/*      (at your option) any later version.                                          */
/*                                                                                   */
/*      Juggluco is distributed in the hope that it will be useful, but              */
/*      WITHOUT ANY WARRANTY; without even the implied warranty of                   */
/*      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                         */
/*      See the GNU General Public License for more details.                         */
/*                                                                                   */
/*      You should have received a copy of the GNU General Public License            */
/*      along with Juggluco. If not, see <https://www.gnu.org/licenses/>.            */
/*                                                                                   */
/*      Fri Jan 27 15:20:04 CET 2023                                                 */


#include "jugglucotext.h"
#ifndef WEAROS
constexpr static std::string_view uklabels[]={"Швидкий","Вуглев","Дэкстр","Довгий","Велос","Прагул","Кров"};
constexpr static Shortcut_t  ukshortinit[]= { {"Bread",
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

jugglucotext uktext {
.daylabel={"Ндл","Пнд","Втр","Срд","Чтв","Птн","Сбт"},

.speakdaylabel={
"неділя",
"понеділок",
"вівторок",
"середа",
"четвер",
"п'ятниця",
"субота"
},
.monthlabel={"Сiч","Лют","Бер","Квi","Тра","Чер","Лип","Сер","Вер","Жов","Лис","Гру"},
.scanned="Сканувати",
.readysecEnable="Датчик готовий через %d хвилин. Скануйте знову, щоб увімкнути потокове передавання.",
.readysec="Датчик готовий через %d хвилин.",
.networkproblem="Проблема з мережею?",
.enablebluetooth="Увімкніть Bluetooth",
.useBluetoothOff="'Використовуйте Bluetooth' вимк",
.noconnectionerror=": Немає з'єднання",
.stsensorerror=": Помилка датчика",
.streplacesensor=": Замінити датчик?",
.endedformat="%s припинив працювати. state=%d",
.notreadyformat="%s не готовий. state=%d",
#ifndef WEAROS
	.median="Медіана",
	.middle="Середній",
#endif
	.history="Історія",
	.historyinfo="Раз у 15 хвилин.\nЗапам'ятовується на датчику 8 годин.\nСканування передає їх до цієї програми.\nДатчик: ",
	.history3info="Раз у 5 хвилин.\nЗапам'ятовується на датчику 14 днів.\nПередано через Bluetooth до цієї програми.\nДатчик: ",
	.sensorstarted= "Датчик запущений:",
	.lastscanned="Останнє сканування:",
	.laststream="Останній потік:",
	.sensorends="Датчик закінчується: ",
#ifndef WEAROS
	.newamount="Нова сума",
	.averageglucose="Середня глюкоза: ",
	.duration="Тривалість: %.1f днів",
	.timeactive="%.1f%% активного часу",
	.nrmeasurement="Кількість вимірювань: %d",
	.EstimatedA1C="Оцінно A1C: %.1f%% (%d mmol/mol)",
	.GMI="Індикатор контролю рівня глюкози (GMI): %.1f%% (%d mmol/mol)",
	.SD="SD: %.2f",
	.glucose_variability="Варіабельність глюкози: %.1f%%",
     .menustr0={
		"System UI        ",
		"Меню",
		"Дивитися",
		"Датчик",
		"Налашт.",
		"Аб Juggluco",
		"Закрити",
		"Стоп тривоги"
		},
	.menustr1={
		"Експорт",
		"Клон",
		uktext.newamount,
		"Список",
		"Статистика",
		"Говорити",
		"Плавати        "


		},
	.menustr2= {"Останнє скан.","Скани","Потік","Історія","Суми","Їжа","Dark mode        "},
	.menustr3= {hourminstr,"Пошук","Дата","День назад","Через день","Тиждень тому","Тиждень пізн"},
#else
 .amount="Сума",
 .menustr0= {
	"Клон",
	"Датчик",
	"    Darkmode      ",
        "Налашт.",
	"Стоп тривоги" },
.menustr2= {"Дата",hourminstr,
"День назад         ",uktext.amount},
#endif

	.scanerrors={
		{"Помилка сканування (%d)","Спробуйте знову"},
		{"Помилка встановлення","?"},
		{"Помилка обробки даних","Спробуйте знову"},
		{"Активація датчика",""},
		{"Датчик точно закінчився",""},

		{"Датчик готовий","%d хвилин"},
		{"Помилка датчика (373)","Не звертайтеся відразу до служби підтримки клієнтів Abbott; можливо, показники рівня глюкози доступні через 10 хвилин."},
		{"Новий датчик активовано","Відскануйте ще раз, щоб використати його"},
		{"","Блокує дотик під час сканування"},
		{"",""},
		{"Помилка ініціалізації бібліотеки","Чи відсутні спільні бібліотеки?"},
		{"Помилка ініціалізації класу","Робити щось"},
		{"Процедура триває надто довго","Я вбиваю програму"},
		{"Замініть датчик (365)","Ваш датчик не працює. Видаліть датчик і запустіть новий."},
		{"Замініть датчик (368)","Ваш датчик не працює. Видаліть датчик і запустіть новий."},
		{"",""},
		{"Помилка сканування","Спробуйте знову"}},

.libre3scanerror={"FreeStyle Libre 3, Помилка сканування", "Спробуйте знову"},
.libre3wrongID={"Помилка, неправильный ID?","Вкажіть у Налашт.->Libreview той самий обліковий запис, який використовувався для активації датчика"},
.libre3scansuccess= {"FreeStyle Libre 3 датчик", "Тепер Juggluco отримуватиме значення глюкози"},
.unknownNFC={"Нерозпізнана помилка сканування NFC", "Спробуйте знову"},
.nolibre3={"FreeStyle Libre 3 датчик","Не підтримується цією версією Juggluco"},
#ifndef WEAROS
	.advancedstart= R"(<h1>Модифікований пристрій</h1>
<p>Одна з бібліотек, які використовуються цією програмою,
має ПОМИЛКУ, яка призводить до збою, якщо вона виявляє певні файли.
Ваш пристрій містить деякі з цих файлів. Ця програма містить хак для
обходу цієї ПОМИЛКИ, але, ймовірно, краще зробити ці файли такими, що
їх неможливо виявити, іншим способом. Magisk, наприклад, має можливість
приховати root для певних програм (Magiskhide або Denylist) і змінити
власну назву, обидва необхідні. У вашому випадку є проблеми з наступним файлом)",
	.add_s=true,
.shortinit=ukshortinit,
.labels=uklabels,
.checked="перевірено",
.unchecked="не перевірено",
.Undetermined="",
.FallingQuickly="Швидко падає",
.Falling="Падіння",
.Stable="Змінюється повільно",
.Rising="Підйом",
.RisingQuickly="Швидко піднімається"
#endif
		}


		;

extern void setuseuk() ;
void setuseuk() {
 usedtext= &uktext;
 }
