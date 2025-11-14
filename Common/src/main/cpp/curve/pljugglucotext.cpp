#include "jugglucotext.hpp"
#ifdef INJUGGLUCO
#ifndef WEAROS
constexpr static std::string_view pllabels[]={
"Węglowodan",
"Dextro",
"Insul szybk",
"Insul dług",
"Rower",
"Spacer",
"Krew"};
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
#endif //INJUGGLUCO

extern jugglucotext pltext;
jugglucotext pltext {
	.daylabel={"nie",
	"pon",
	"wto",
	"śro",
	"czw",
	"pią",
	"sob"},
#ifdef INJUGGLUCO
.speakdaylabel={ "niedziela","poniedziałek","wtorek","środa","czwartek","piątek","sobota"},
	.monthlabel={
      "sty",
      "lut",
      "mar",
      "kwi",
      "maj",             
      "cze",
       "lip",
       "sie",
       "wrz",
      "paź",
      "lis",
      "gru"},

	.scanned="Zeskanowano",
	.readysecEnable="Sensor gotowy za %d minut(y). Zeskanuj go ponownie, by włączyć przesył danych co 1 min.",
	.readysec="Sensor gotowy za %d minut(y).",
.networkproblem="Brak danych ze źródła",
.enablebluetooth="Włącz Bluetooth",
.useBluetoothOff="'Użyj Bluetooth' wył.",
.noconnectionerror=": Brak połączenia",
.stsensorerror=": Błąd odczytu sensora",
.streplacesensor=": Wymienić sensor?",
.endedformat="Sensor %s przestał działać. patchState=%d",
.notreadyformat="%s nie jest jeszcze gotowy. state=%d",
#ifndef WEAROS
	.median="Mediana",
	.middle=" Średnia",
#endif
	.history="Historia",
//	 historyinfo="Raz na 15 minut.\nSensor zapamiętuje wartości przez 8 godzin.\nSkanowanie przesyła je do aplikacji.\nSensor: ", 
//	.history3info="Raz na 5 minut.\nSensor zapamiętuje wartości przez 14 dni.\nSą one przesyłane przez Bluetooth do tej aplikacji.\nSensor: ",
	.sensorstarted= "Początek:",
	.lastscanned="Ostatni skan:",
	.laststream="Ostatni strumień:",
	.sensorends="Oficjalny koniec: ",
	.sensorexpectedend="Koniec po przedł.: ",
#endif //INJUGGLUCO
#ifndef WEAROS
#ifdef INJUGGLUCO
	.newamount="Nowa wartość",
	.averageglucose="Średnie stęż. glukozy: ",
	.duration="Okres aktywności: %.1f dni",
	.timeactive="%.1f%% czasu aktywności",
	.nrmeasurement="Liczba pomiarów: %d",
	.EstimatedA1C="Szacowana wartość HbA1C: %.1f%% (%d mmol/mol)",
	.GMI="Wskaźnik zarządzania poziomem glukozy (GMI): %.1f%% (%d mmol/mol)",
	.SD="Odchylenie standardowe: %.2f",
	.glucose_variability="Zmienność stęż. glukozy: %.1f%%",
     .menustr0={
		"Integr. z sys.        ",
		"Całe menu",
		"Zegarek",
		"Sensor",
		"Ustawienia",

#if defined(SIBIONICS)
		"Zdjęcie",
#else
		"Informacje",
#endif

		"Zamknij",
		"Zatrzymaj alarm"
		},
	.menustr1={
		"Eksport",
		"Klonowanie",
		pltext.newamount,
		"Lista", 
#else
       .statistics=
#endif //INJUGGLUCO
		"Statystyka",
#ifdef INJUGGLUCO
		"Na głos",
		"Pływ. wart."
		},
	.menustr2= {"Calibrated",
	"Skany",
	"Strumień",
	"Historia",
	"Wartości",
	"Posiłki",
   "Tryb ciemny        "},
	.menustr3= {hourminstr,
	"Wyszukaj",
	"Data",
	"Dzień wstecz",
	"Dzień później",
	"Tydzień wstecz",
	"Tydzień później"},
#endif //INJUGGLUCO
#else
 .amount="Wartość",
 .menustr0= {
	"        Klon",
	"Sensor",
	"Ekran",
        "Ustawienia",
	"Zatrzym. alarm"},
.menustr2= {"Data",
hourminstr, "Dzień wstecz              ",pltext.amount},
#endif

#ifdef INJUGGLUCO
	.scanerrors={
		{"Błąd skanowania (%d)", "Spróbuj ponownie"},
		{"Błąd instalacji", "?"},
		{"Błąd przetwarzania danych", "Spróbuj ponownie"},
		{"Uruchamianie sensora", ""},
		{"Sensor zakończył działanie", ""},
		{"Sensor gotowy za", "%d minut(y)"},
		{"Błąd sensora (373)", "Spróbuj ponownie za 10 minut"},
		{"Nowy sensor uruchomiony", "Zeskanuj go ponownie, by z niego korzystać"},
		{"", "Blokuje funkcje dotykowe w czasie skanowania"},
		{"", ""},
		{"Błąd podczas ładowania biblioteki", "Czy brakuje bibliotek współdzielonych?"},
		{"Błąd podczas ładowania klasy", "Zrób coś"},
		{"Procedura trwa zbyt długo", "Zamykam program"},
		{"Wymień sensor (365)", "Twój sensor nie działa. Usuń ten sensor i uruchom nowy."},
		{"Wymień sensor (368)", "Twój sensor nie działa. Usuń ten sensor i uruchom nowy."},
		{"", ""},
		{"Błąd skanowania", "Spróbuj ponownie"}},

.libre3scanerror={"FreeStyle Libre 3, Błąd skanowania", 
	"Spróbuj ponownie"},
.libre3wrongID={"Błąd, niepoprawny nr ID konta?",
	R"(Powinien być taki sam jak podczas aktywacji sensora). Naciśnij Lewe menu->Ustawienia->Przesył danych->Libreview->"Pobierz identyfikator konta", aby pobrać go z Libreview)"},
.libre3scansuccess={"Sensor FreeStyle Libre 3", 
	"Wartości stężenia glukozy będą teraz odbierane przez Juggluco"},
.unknownNFC={"Nieznany błąd skanowania NFC", 
	"Spróbuj ponownie"},
.nolibre3={"Sensor FreeStyle Libre 3",
	"Pobierz poprawną wersję ze strony https://www.juggluco.nl/download.html"},
.libre3zeroID={"Error, zero account ID?",
	R"(Use Left menu->Settings->Exchange data->Libreview->"Get Account ID" to set a non-zero account ID.)"},
.needsandroid8="Wymaga co najmniej systemu Android 8 "sv,
#ifndef WEAROS
	.advancedstart= R"(<h1>Urządzenie zmodyfikowane</h1>
<p>Jedna z bibliotek używanych przez tę aplikację ma BŁĄD, który powoduje, że się zawiesza, jeśli wykryje pewne pliki.
 Twoje urządzenie zawiera niektóre z tych plików.
 Ten program zawiera sposób na obejście tego błędu, ale pewnie lepiej jest zrobić, by pliki te były niewykrywalne w inny sposób.

 Na przykład Magisk ma opcję ukrycia roota dla niektórych aplikacji (Magiskhide lub Denylist) i zmiany własnej nazwy, oba są potrzebne.
 
 W twoim przypadku ma problemy z następującym plikiem)",
	.add_s=true,
.shortinit=plshortinit,
.labels=pllabels,
#endif
#ifndef DONTTALK
.checked="zaznaczono",
.unchecked="nie zaznaczono",
.Undetermined=""sv,
.FallingQuickly="Szybki spadek"sv,
.Falling="Spadek"sv,
.Stable="Powolna zmiana"sv,
.Rising="Wzrost"sv,
.RisingQuickly="Szybki wzrost"sv,
#endif
.receivingpastvalues="Odbiór starych wartości"sv,
.receivingdata="Odbiór danych"sv,
.unsupportedSibionics="Nieobsługiwany sensor Sibionics"sv,
.waitingforconnection="Czekam na połączenie"sv,
.deleted="Usunięto"sv,
.nolocationpermission="Wymaga uprawnienia do lokalizacji"sv,
.nonearbydevicespermission="Wymaga uprawnienia do wykrywania urządzeń w pobliżu"sv
#endif //INJUGGLUCO
,.summarygraph="Wykres zbiorczy"sv
,.logdays="Dni"sv
,.unhide="Unhide"sv
		};

addlang(pl);
