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
constexpr static std::string_view pllabels[]={"Insul szybk","Węglowodan","Dextro","Insul dług","Rower","Spacer","Krew"};
constexpr static Shortcut_t  plshortinit[]= { {"Chleb",
        .48},
        {"Bułka1",
        .56f},
        {"Bułka2",
        .595f},
        {"Winogrona",
        .165f},
        {"Jogurt owoc",
        .058f},
        {"Ryż",
        .75f},
        {"Makaron",
        .65f},
        {"Pomidor",
        .03f},
        {"Miks mex",
        .078f},
        {"Sok pomar",
        .109f},
        {"Odż. spor",
        .873f},
        {"Miks (mar)",
        .07f},
        {"Miks grzyb",
        .07300000f}};
#endif

extern jugglucotext pltext;
jugglucotext pltext {
	.daylabel={"nie","pon","wto","śro","czw","pią","sob"},
	.monthlabel={
      "sty","lut","mar","kwi","maj","cze","lip","sie","wrz","paź","lis","gru"},

	.scanned="Zeskanowane",
		.readysecEnable="Sensor gotowy za %d minut. Zeskanuj go ponownie, by umożliwić przesył danych.",
		.readysec="Sensor gotowy za %d minut.",
		.networkproblem="Problem z siecią?",
		.enablebluetooth="Włącz Bluetooth",
		.useBluetoothOff="'Użyj Bluetooth' wył.",
		.noconnectionerror=": Brak połączenia",
		.stsensorerror=": Błąd sensora",
		.streplacesensor=": Wymienić sensor?",
.endedformat="Sensor %s not working anymore. patchState=%d",
.notreadyformat="Sensor %s not ready. patchState=%d",

#ifndef WEAROS
	.median="Mediana",
	.middle="Średnia",
#endif
	.history="Historia",
	.historyinfo="Raz na 15 minut.\nZapamiętana na sensorze przez 8 godzin.\nSkanowanie przesyła je do aplikacji.\nSensor: ",
	.history3info="Raz na 5 minut.\nZapamiętana na sensorze przez 14 dni.\nPrzesyłana przez Bluetooth do tej aplikacji.\nSensor: ",
	.sensorstarted= "Początek sensora:",
	.lastscanned="Ostatni odczyt:",
	.laststream="Ostatni strumień:",
	.sensorends="Koniec sensora: ",
#ifndef WEAROS
	.newamount="Nowa wartość",
	.averageglucose="Średnie stęż. glukozy: ",
	.duration="Czas aktywności: %.1f dni",
	.timeactive="%.1f%% czasu aktywności",
	.nrmeasurement="Liczba pomiarów: %d",
	.EstimatedA1C="Szacowana wartość HbA1C: %.1f%% (%d mmol/mol)",
	.GMI="Wskaźnik pomiaru glukozy (GMI): %.1f%% (%d mmol/mol)",
	.SD="Odchylenie standardowe: %.2f",
	.glucose_variability="Zmienność stęż. glukozy: %.1f%%",
     .menustr0={
		"Integr. z sys.        ",
		"Zegarek",
		"Sensor",
		"Ustawienia",
		"Informacje",
		"Zamknij",
		"Zatrzymaj alarm"
		},
	.menustr1={
		"Eksport",
		"Klonowanie",
		pltext.newamount,
		"Lista", 
		"Statystyka",
		"Talk"
		},
	.menustr2= {"Ostatni odczyt","Odczyty","Strumień","Histora","Wartość","Posiłki","Tryb ciemny        "},
	.menustr3= {hourminstr,"Wyszukaj","Data","Dzień wstecz","Dzień później","Tydzień wstecz","Tydzień później"},
#else
 .amount="Wartość",
 .menustr0= {
	"Klon",
	"Sensor",
	"Tr. ciemny      ",
    "Ustawienia",
	"Zatrzymaj alarm" },
.menustr2= {"Data",hourminstr,"1 d. wstecz",pltext.amount},
#endif

	.scanerrors={
		{"Błąd odczytu(%d)","Spróbuj ponownie"},
		{"Błąd instalacji","?"},
		{"Błąd przetwarzania danych","Spróbuj ponownie"},
		{"Aktywacja sensora",""},
		{"Sensor ostatecznie zakończył działanie",""},

		{"Sensor gotowy za","%d minut"},
		{"373: Błąd sensora","Nie kontaktuj się od razu z obsługą klienta firmy Abbott; odczyty glukozy mogą być dostępne za 10 minut."},
		{"Nowy sensor uruchomiony","Zeskanuj go ponownie, by z niego korzystać"},
		{"","Blokuje dotyk w czasie skanowania"},
		{"",""},
		{"Błąd instalacji biblioteki","Wymuś reinstalację poprzez usunięcie biblioteki"},
		{"Błąd inicjalizacji klasy", "Zrób coś"},
		{"Procedura trwa zbyt długo","Zamykam program"},
		{"365: Wymień sensor","Twój sensor nie działa. Proszę usunąć sensor i założyć nowy."},
		{"368: Wymień sensor","Twój sensor nie działa. Proszę usunąć sensor i założyć nowy."},
		{"",""},
		{"Błąd odczytu","Spróbuj ponownie"}},

#ifndef WEAROS
	.advancedstart= R"(<h1>Zmodyfikowane urządzenie</h1>
<p>Jedna z bibliotek używanych przez tę aplikację ma BŁĄD, który powoduje, że się zawiesza, jeśli wykryje pewne pliki. Twoje urządzenie zawiera niektóre z tych plików. Ten program zawiera sposób na obejście tego błędu, ale prawdopodobnie lepiej jest zrobić, pliki te były niewykrywalne w inny sposób. Na przykład Magisk ma opcję ukrycia roota dla niektórych aplikacji (Magiskhide lub Denylist) i zmiany własnej nazwy, oba są potrzebne. W twoim przypadku ma problemy z następującym plikiem)",
	.add_s=true,
.shortinit=plshortinit,
.labels=pllabels
#endif
		}


		;


void setusepl() {
 usedtext= &pltext;
 }
