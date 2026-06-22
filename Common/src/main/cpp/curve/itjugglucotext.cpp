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


#include "jugglucotext.hpp"
#ifdef INJUGGLUCO
#ifndef WEAROS
constexpr static std::string_view itlabels[]= {"Carboidrati","Glucosio","Rapida","Lenta","Bike","Walk","Capillare"};
constexpr static Shortcut_t  itshortinit[]= { {"Muffin", .54f},

{"Uva",

.165f},

{"YogFrutta",

.058f},

{"Riso", .75f},

{"Pasta", .65f},

{"Pomodoro",

.03f},

{"Messicano",

.078f},

{"SuccoAranci",

.109f},

{"Mix(Carote)",

.07f},

{"Mix funghi",

.07300000f}};
#endif
#endif //INJUGGLUCO
static jugglucotext ittext {
	.daylabel={"dom","lun","mar","mer","gio","ven","sab"},

#ifdef INJUGGLUCO
.speakdaylabel={
"Domenica",
"Lunedì",
"Martedì",
"Mercoledì",
"Giovedì",
"Venerdì",
"Sabato"},
	.monthlabel={"gen", "feb", "mar", "apr", "mag", "giu", "lug", "ago", "set", "ott", "nov", "dic"}, 
	.scanned="Scansionato",
	.readysecEnable="Sensore pronto tra %d minuti. Scansiona di nuovo per abilitare lo Streaming.",
	.readysec="Sensore pronto tra %d minuti.",
.networkproblem="Nessun glucosio dal mirror",
.enablebluetooth="Abilita Bluetooth",
.useBluetoothOff="'Usa Bluetooth' disattivato",
.noconnectionerror=": Nessuna connessione",
.stsensorerror=": Errore sensore",
.streplacesensor=": Sostituire il sensore?",
.endedformat="%s ha smesso di funzionare. stato=%d",
.notreadyformat="%s non pronto. stato=%d",
#ifndef WEAROS
	.median="Mediana",
	.middle="Centro",
#endif
	.history="Cronologia",
	.sensorstarted= "Avviato:\t\t\t\t\t\t",
	.lastscanned="Ultima scansione:",
	.laststream="Ultimo flusso:\t\t",
	.sensorends="Termina ufficialmente: ",
	.sensorexpectedend="Fine prevista: ",
#endif //INJUGGLUCO
#ifndef WEAROS
#ifdef INJUGGLUCO
	.newamount="Nuovo valore",
	.averageglucose="Glucosio medio: ",
	.duration="Durata: %.1f giorni",
	.timeactive="%.1f%% del tempo attivo",
	.nrmeasurement="Numero di misure: %d",
	.EstimatedA1C="A1C stimata: %.1f%% (%d mmol/mol)",
	.GMI="Glucose Management Indicator: %.1f%% (%d mmol/mol)",
	.SD="SD: %.2f",
	.glucose_variability="Variabilità glicemica: %.1f%%",
     .menustr0={
		"UI di sistema",
		"Menu",
		"Orologio",
		"Sensore",
		"Impostazione",
#if defined(SIBIONICS)
		"Foto",
#else
		"About",
#endif
		"Chiudi",
		"Stop Allarme"
		},
	.menustr1={
		"Esporta",
		"Mirror",
		ittext.newamount,
		"Lista", 
#else
       .statistics=
#endif //INJUGGLUCO
		"Statistiche",
#ifdef INJUGGLUCO
		"Pronuncia",
		"Flottante"
		},
	.menustr2= {"Ultima scansione","Scansioni","Flusso","Cronologia","Valori","Pasti","Modalità scura"}, 
	.menustr3= {hourminstr,"Cerca","Data","Giorno prima","Giorno dopo","Settimana prima","Settimana dopo"}, 
#endif //INJUGGLUCO
#else
	.amount="Valore",
 .menustr0= {
	"Mirror",
	"Sensore",
	"Display",
	"Impostazione",
	"Stop Allarme"
	 },
   .menustr2= {"Data",hourminstr,"Giorno prima",ittext.amount},
#endif
#ifdef INJUGGLUCO
	.scanerrors={
		{"Errore di scansione (%d)","Riprova"},
		{"Errore di installazione","?"},
		{"Errore di analisi dei dati","Riprova"},
		{"Attivazione sensore",""},
		{"Sensore definitivamente scaduto",""},
		{"Sensore pronto tra","%d minuti"},
		{"Errore del sensore (373)","Riprova più tardi"},
		{"Nuovo sensore inizializzato","Scansiona di nuovo per usarlo"},
		{"","Blocca il touch durante la scansione"},
		{"",""},
		{"Errore di inizializzazione della libreria","Mancano librerie condivise?"},
		{"Errore di inizializzazione della classe","Fai qualcosa"}, 
		{"La procedura sta impiegando troppo tempo","Chiudo il programma"},
		{"Sostituire il sensore (365)","Il tuo sensore non funziona. Rimuovilo e avviane uno nuovo."},
		{"Sostituire il sensore (368)","Il tuo sensore non funziona. Rimuovilo e avviane uno nuovo."},
		{"",""},
		{"Errore di scansione","Riprova"}},

.libre3scanerror={"FreeStyle Libre 3, errore di scansione", "Riprova"},
.libre3wrongID={"Errore, account ID errato?",
	R"(Deve essere lo stesso usato per attivare il sensore. Per recuperarlo da LibreView, vai in Menu sinistro→Impostazione→Scambio dati→LibreView→"Ottieni Account ID")"},
.libre3scansuccess= {"Sensore FreeStyle Libre 3", "I valori del glucosio saranno ora ricevuti da Juggluco"},
.unknownNFC={"Errore: scansione NFC non riconosciuta", "Riprova"},
.nolibre3={"Sensore FreeStyle Libre 3","Scarica la versione corretta da https://www.juggluco.nl/download.html"},
.libre3zeroID={"Errore, account ID a zero?",
	R"(Usa Menu sinistro->Impostazione->Scambio dati->Libreview->"Ottieni Account ID" per impostare un account ID diverso da zero.)"},
.needsandroid8="Richiede almeno Android 8"sv,
#ifndef WEAROS
	.advancedstart=R"(<H1>Dispositivo modificato</H1>
<P>Una delle librerie usate dall'applicazione Juggluco ha un BUG che
la fa crashare se vengono rilevati determinati file. Il tuo
dispositivo contiene alcuni di questi file. Questo programma
contiene un trucco per aggirare questo BUG, ma
sarebbe comunque meglio rendere questi file non rilevabili in altri
modi. Magisk, per esempio, ha l'opzione per nascondere root ad alcune
applicazioni (Magiskhide oppure
Denylist) e cambiare il proprio nome, entrambe queste attenzioni
sono necessarie. Nel tuo caso c'&egrave; un problema con il
seguente file</P>)",
	.add_s=false,
.shortinit=itshortinit,
.labels=itlabels,
#endif
#ifndef DONTTALK
.checked="selezionato",
.unchecked="non selezionato",
.Undetermined=""sv,
.FallingQuickly="In rapida diminuzione"sv,
.Falling="In diminuzione"sv,
.Stable="In lenta modifica"sv,
.Rising="In aumento"sv,
.RisingQuickly="In rapido aumento"sv,
#endif
.receivingpastvalues="Ricezione vecchi valori"sv,
.receivingdata="Ricezione dati"sv,
.unsupportedSibionics="Sensore Sibionics non supportato"sv,
.waitingforconnection="In attesa di connessione"sv,
.deleted="Cancellato"sv,
.nolocationpermission="Serve il permesso di posizione"sv,
.nonearbydevicespermission="Serve il permesso dispositivi vicini"sv,
#endif //INJUGGLUCO
.summarygraph="Grafico riassuntivo"sv
,.logdays="Log giornaliero"sv
,.unhide="Mostra"sv


}
		;
addlang(it);

