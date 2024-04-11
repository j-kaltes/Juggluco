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
extern jugglucotext engtext;
#ifndef WEAROS
constexpr static std::string_view itlabels[]= {"Rapida","Carboidrati","Glucosio","Lenta","Bike","Walk","Capillare"};
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
static jugglucotext ittext {
	.daylabel={"dom","lun","mar","mer","gio","ven","sab"},

.speakdaylabel={
"Domenica",
"Lunedi",
"Martedì",
"Mercoledì",
"Giovedì",
"Venerdì",
"Sabato"},
	.monthlabel={"gen", "feb", "mar", "apr", "mag", "giu", "lug", "ago", "set", "ott", "nov", "dic"}, 
	.scanned="Scansione",
	.readysecEnable="Sensor ready in %d minutes. Scan again to enable Streaming.",
	.readysec="Sensor ready in %d minutes.",
.networkproblem="Network problem?",
.enablebluetooth="Enable Bluetooth",
.useBluetoothOff="'Use Bluetooth' off",
.noconnectionerror=": No Connection",
.stsensorerror=": Sensor Error",
.streplacesensor=": Replace Sensor?",
.endedformat="Sensor %s not working anymore. state=%d",
.notreadyformat="Sensor %s not ready. state=%d",
#ifndef WEAROS
	.median="Mediana",
	.middle="Centro",
#endif
	.history="Cronologia",
	.historyinfo="Una volta per 15 minuti, Ricordarsi per 8 ore.\nScansionare li trasferirà al programma.\nSensore: ",
	.history3info=engtext.history3info,
	.sensorstarted= "Sensore avviato il:",
	.lastscanned="Ultima scansione:",
	.laststream="Ultima scansione:",
	.sensorends="Il sensore teminerà il: ",
#ifndef WEAROS
	.newamount="Nuovo valore",
	.averageglucose="Media glicemica: ",
	.duration="Duarata: %.1f giorni",
	.timeactive="%.1f%% di tempo attivo",
	.nrmeasurement="Numero di misure: %d",
	.EstimatedA1C="A1C stimata: %.1f%% (%d mmol/mol)",
	.GMI="Glucose Management Indicator: %.1f%% (%d mmol/mol)",
	.SD="SD: %.2f",
	.glucose_variability="Variabilità glicemica: %.1f%%",
     .menustr0={
		"UI di sistema       ",
		"Menus",
		"Orologio",
		"Sensore",
		"Impostazione",
#if defined(SIBIONICS)
"Sibionics",
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
		"Statistiche",
		"Talk",
		"Float       "
		},
	.menustr2= {"Ultima scansione","Scansioni","Flusso","Cronologia","Valori","Pasti","Modalità scura       "}, 
	.menustr3= {hourminstr,"Cerca","Data","Giorno prima","Giorno dopo","Settimana prima","Settimana dopo"}, 
#else
	.amount="Valore",
 .menustr0= {
	"Mirror",
	"Sensore",
	"    Darkmode      ",
	"Impostazione",
	"Stop Allarme"
	 },
   .menustr2= {"Data",hourminstr,"Giorno dietro       ",ittext.amount},
#endif
	.scanerrors={
		{"Scan Error (%d)","Riprova"},
		{"Errore di installazione","?"},
		{"Errore di analisi dei dati","Riprova"},
		{"Attivazione sensore",""},
		{"Sensore definitivamente scaduto",""},
		{"Sensore pronto in","%d minuti"},
		{"Errore del sensore (373)","Non contattare l'Abbotto subito, esiste la possibilità che riprenda a leggere dopo 10 minuti."},
		{"Nuovo sensore inizializzato","Scansionare di nuovo per usarlo"},
		{"","Bloccare il touch durante la scansione"},
		{"",""},
		{"Errore di inizializzazione della libreria","Reinstallazione forzata per rimozione libreria"},
		{"Errore di inizializzazione della classe","Fai qualcosa"}, 
		{"La procedura sta impiegando troppo tempo","Chiuderò il programma"},
		{"Sostituire sensore (365)","Il tuo sensore non funziona. Rimuovilo e avviane un altro."},
		{"Sostituire sensore (368)","Il tuo sensore non funziona. Rimuovilo e avviane un altro."},
		{"",""},
		{"Errore di scansione","Riprova"}},

.libre3scanerror={"FreeStyle Libre 3, Scan error", "Try again"},
.libre3wrongID={"Error, wrong account ID?","Specify in Settings->Libreview the same account used to activate the sensor"},
.libre3scansuccess= {"FreeStyle Libre 3 sensor", "Glucose values will now be received by Juggluco"},
.unknownNFC={"Unrecognized NFC scan Error", "Try again"},
.nolibre3={"FreeStyle Libre 3 sensor","Not supported by this version of Juggluco"},
#ifndef WEAROS
	.advancedstart=R"(<H1>Dispositivi modificati</H1>
<P>Una delle librerie usate dall'applicazione Juggluco ha un BUG che
la fa crashare se vengono rilevati determinati file. Il tuo
dispositivo potrebbe contenerli nella sua memoria. Questo programma
contiene una peculiarit&agrave; tecnica che previene questo BUG, ma
sarebbe comunque meglio rendere questi file non rilevabili in alcuni
altri modi. Magisk, per esempio, ha l'opzione per nascondere la
cartella radice ad alcune altre applicazioni (Magiskhide oppure
Denylist) e cambia il suo stesso nome, entrambe queste attenzioni
sono necessaire.<BR>Nel tuo caso c'&egrave; un problema con i
seguenti file)",
	.add_s=false,
.shortinit=itshortinit,
.labels=itlabels,
.checked="checked",
.unchecked="not checked",
.Undetermined="",
.FallingQuickly="In rapida diminuzione",
.Falling="In diminuzione",
.Stable="In lenta modifica",
.Rising="In aumento",
.RisingQuickly="In rapido aumento",
#endif
.receivingpastvalues="Receiving old values"
}
		;
#ifdef JUGGLUCO_APP
#include "logs.h"
#else
#include <stdio.h>
#define LOGGER(...) fprintf(stderr,__VA_ARGS__)

#endif
extern void setuseit() ;
void setuseit() {
	usedtext= &ittext;
	LOGGER("history3info=%s\n",ittext.history3info.data());
	}

