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
.speakdaylabel={
	"Niedziela",
"Poniedziałek",
"Wtorek",
"Środa",
"Czwartek",
"Piątek",
"Sobota"},
	.monthlabel={"sty","lut","mar","kwi","maj","cze","lip","sie","wrz","paź","lis","gru"},
       
      

	.scanned="Zeskanowano",
	.readysecEnable="Sensor gotowy za %d minut(y). Zeskanuj go ponownie, by włączyć przesył danych co 1 min.",
	.readysec="Sensor gotowy za %d minut(y).",
.networkproblem="Brak danych ze źródła",
.enablebluetooth="Włącz Bluetooth",
.useBluetoothOff="'Użyj Bluetooth' wył.",
.noconnectionerror=": Brak połączenia",
.stsensorerror=": Błąd sensora",
.streplacesensor=": Wymienić sensor?",
.endedformat="Sensor %s przestał działać. patchState=%d",
.notreadyformat="%s nie jest jeszcze gotowy. state=%d",
#ifndef WEAROS
	.median="Mediana",
	.middle=" Średnia",
#endif
	.history="Historia",
	.historyinfo="Raz na 15 minut.\nSensor zapamiętuje wartości przez 8 godzin.\nSkanowanie przesyła je do aplikacji.\nSensor: ",
	.history3info="Raz na 5 minut.\nSensor zapamiętuje wartości przez 14 dni.\nSą one przesyłane przez Bluetooth do tej aplikacji.\nSensor: ",
	.sensorstarted= "Początek sensora:",
	.lastscanned="Ostatni odczyt:",
	.laststream="Ostatni strumień:",
	.sensorends="Koniec sensora: ",
#ifndef WEAROS
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
		"Na głos",
		"Pływ. wart.        "
		},
	.menustr2= {"Ostatni odczyt","Skany","Strumień","Historia","Wartości","Posiłki","Tryb ciemny        "},
	.menustr3= {hourminstr,"Wyszukaj","Data","Dzień wstecz","Dzień później","Tydzień wstecz","Tydzień później"},
#else
 .amount="Wartość",
 .menustr0= {
	"Klon",
	"Sensor",
	"   Tr. ciemny      ",
        "Ustawienia",
	"Zatrzym. alarm"},
.menustr2= {"Data",hourminstr, "1 d. wstecz            ",pltext.amount},

#endif

	.scanerrors={
		{"Błąd skanowania (%d)","Spróbuj ponownie"},
		{"Błąd instalacji","?"},
		{"Błąd przetwarzania danych","Spróbuj ponownie"},
		{"Uruchomienie sensora",""},
		{"Sensor zakończył działanie",""},

		{"Sensor gotowy za","%d minut(y)"},
		{"373: Błąd odczytu sensora","Spróbuj za 10 minut"},
		{"Nowy sensor uruchomiony","Zeskanuj go ponownie, by z niego korzystać"},
		{"","Blokuje funkcje dotykowe w czasie skanowania"},
		{"",""},
		{"Błąd podczas ładowania biblioteki", "Czy brakuje bibliotek współdzielonych?"},
		{"Błąd podczas ładowania klasy", "Zrób coś"},
		{"Procedura trwa zbyt długo","Zamykam program"},
		{"365: Wymień czujnik","Twój czujnik nie działa. Usuń czujnik i uruchom nowy."},
		{"368: Wymień czujnik","Twój czujnik nie działa. Usuń czujnik i uruchom nowy."},
		{"",""},
		{"Błąd skanowania", "Spróbuj ponownie"}},

.libre3scanerror={"FreeStyle Libre 3, Błąd skanowania", "Spróbuj ponownie"},
.libre3wrongID={"Błąd, niepoprawny nr ID konta?", "W menu Ustawienia ->Libreview wpisz to samo konto, które zostało użyte do aktywacji sensora"},
.libre3scansuccess={"Sensor FreeStyle Libre 3", "Wartości stężenia glukozy będą teraz odbierane przez Juggluco"},
.unknownNFC={"Nieznany błąd skanowania NFC", "Spróbuj ponownie"},
.nolibre3={"Sensor FreeStyle Libre 3", "Nie jest obsługiwany przez tę wersję Juggluco"},
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
.checked="zaznaczono",
.unchecked="nie zaznaczono",
.Undetermined="",
.FallingQuickly="Szybki spadek",
.Falling="Spadek",
.Stable="Powolna zmiana",
.Rising="Wzrost",
.RisingQuickly="Szybki wzrost"
#endif
		}


		;


void setusepl() {
 usedtext= &pltext;
 }
