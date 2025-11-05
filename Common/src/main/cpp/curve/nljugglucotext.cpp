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
constexpr static std::string_view nllabels[]={  "Koolhydraat","Dextro","Insuli snel", "Insuli lang","Fietsen", "Wandelen","Bloed"};
constexpr static Shortcut_t  nlshortinit[]= { {"Meer. Brood",
        .42f},
{
"Krentbrood1",
        .56f},
        {"Krentbrood2",
        .595f},
        {"Druiven",
        .165f},
        {
"Frucht yog",
        .058f},
        {"Rijst",
        .75f},
        {"Macaroni",
        .65f},
        {"Tomaat",
        .03f},
        {"Mexican mix",
        .078f},
        { "Sinappelsap", .109f},
        {"SportPowder",
        .873f}};
#endif
#endif
static jugglucotext nltext {
	.daylabel={"zo","ma","di","wo","do","vr","za"},
#ifdef INJUGGLUCO
.speakdaylabel={
"Zondag",
"Maandag",
"Dinsdag",
"Woensdag",
"Donderdag",
"Vrijdag",
"Zaterdag"},
	.monthlabel={"jan","feb","mrt","apr","mei","jun","jul","aug","sep", "okt","nov","dec"},

	.scanned="gescanned",
	.readysecEnable="Sensor is gereed in %d minuten. Scan nog een keer om Streaming aan te zetten.",
	.readysec="Sensor is gereed in %d minuten.",
.networkproblem="Geen glucose van kloon",
.enablebluetooth="Zet Bluetooth aan!",
.useBluetoothOff="'Gebruik Bluetooth' uit",
.noconnectionerror=": geen verbinding",
.stsensorerror=": Sensorfout",
.streplacesensor=": vervang sensor?",
.endedformat="Sensor %s not working anymore. state=%d",
.notreadyformat="Sensor %s not ready. state=%d",
#ifndef WEAROS
	.median="Mediaan",
	.middle="Midden",
#endif
	.history="History",
	.historyinfo="Een keer in 15 minuten, onthouden gedurende 8 uur.\nDoor scanning naar Juggluco gezonden.\nSensor: ",
	.history3info="Een keer in 5 minute.\nVia Bluetooth naar Juggluco gezonden.\nSensor: ",
	.sensorstarted= "Gestart:",
	.lastscanned="Laatste scan:",
	.laststream="Laatste stream:",
	.sensorends="Eindigt officieel: ",
      .sensorexpectedend="Verwacht einde: ",
#endif
#ifndef WEAROS
#ifdef INJUGGLUCO
	.newamount="Hoeveelheid",
	.averageglucose="Gemiddelde glucose: ",
	.duration="Duur: %.1f dagen",
	.timeactive="%.1f%% van de tijd actief",
	.nrmeasurement="Aantal metingen: %d",
	.EstimatedA1C="Geschatte Hba1C: %.1f%% (%d mmol/mol)",
	.GMI="Glucose Management Indicator: %.1f%% (%d mmol/mol)",
	.SD="SD: %.2f",
	.glucose_variability="Glucose variabiliteit: %.1f%%",


     .menustr0={
		"System UI        ",
		"Menus",
		"Horloges",
		"Sensor",
		"Instellingen",
#if defined(SIBIONICS)
"Foto",
#else
"About",
#endif
		"Sluiten",
		"Stop Alarm"
		},
	.menustr1={
		"Export",
		"Kloon",
		nltext.newamount,
		"Lijst", 
#else
       .statistics=
#endif //INJUGGLUCO
		"Statistiek",
#ifdef INJUGGLUCO
		"Praat",
		"Zweef        "
		},
	.menustr2= {"Gekalibreerd","Scans","Stream","History","Hoeveelheden        ","Maaltijden","Dark mode"},
	.menustr3= {hourminstr,"Zoeken","Datum","Dag terug","Dag later","Week terug","Week later"},
#endif
#else
 .amount="Hoeveelheid",
 .menustr0= {
	"       Kloon",
	"Sensor",
	"Toon",
        "   Instellingen",
	"Stop Alarm" },
.menustr2= {"Datum",hourminstr, "Dag terug                    ",nltext.amount},
#endif
#ifdef INJUGGLUCO

	.scanerrors={
		{"Scanfout (%d)","Probeer het opnieuw"},
		{"Installatie fout","?"},
		{"Data verwerking fout","Probeer het opnieuw"},
		{"Sensor activeren",""},
		{"Sensor is volledig opgebruikt",""},
		{"Sensor klaar in","%d minuten"},
		{"Sensorfout (373)","Probeer het later nog eens"},
		{"Nieuwe sensor geïnitialiseed","Scan nog eens om hem te gebruiken"},
		{"","Reageert niet op aanraking tijdens scannen"},
		{"",""},
		{"Library initialization error","Are shared libraries missing?"},
		{"Class initalization error","Do something"},
		{"Procedure takes too long","I kill program"},
		{"Vervang sensor (365)","Uw sensor werkt niet. Verwijder uw sensor en start een nieuwe sensor."},
		{"Vervang sensor (368)","Uw sensor werkt niet. Verwijder uw sensor en start een nieuwe sensor."},
		{"",""},
		{"Scanfout","Probeer het opnieuw"}},


.libre3scanerror={"FreeStyle Libre 3, Scanfout", "Probeer het opnieuw"},
.libre3wrongID={"Fout, verkeerde account-ID?",
R"(Moet hetzelfde zijn als bij het activeren van de sensor. Gebruik Linkermenu->Instellingen->Deel gegevens->Libreview->"Account-ID ophalen" om het te halen van Libreview.)"},
.libre3scansuccess= {"FreeStyle Libre 3-sensor", "Glucosewaarden worden nu ontvangen door Juggluco"},
.unknownNFC={"Onbekende NFC-scanfout", "Probeer het opnieuw"},
.nolibre3={"FreeStyle Libre 3-sensor","Download de juiste versie van https://www.juggluco.nl/download.html"},
.libre3zeroID={"Fout, nul account-ID?",
R"(Gebruik Linkermenu->Instellingen->Deel gegevens->Libreview->"Account-ID ophalen" voor het invoeren van een account ID.)"},
.needsandroid8="Needs minimally Android 8"sv,
#ifndef WEAROS
	.advancedstart= R"(<h1>Veranderd apparaat</h1>
<p>Een van de libraries
gebruikt door deze applicatie bevat een BUG die maakt dat het crashed
wanneer het bepaalde bestanden ontdekt. Dit programma gebruikt een
hack om deze BUG te omzeilen, maar het is waarschijnlijk beter deze
files op een andere manier onvindbaar te maken. Magisk heeft
bijvoorbeeld de mogelijkheid root te verbergen voor bepaalde
applicaties (Magiskhide of Denylist) en zijn eigen naam te
veranderen. Beide zijn nodig. In het huidige geval betreft het de
volgende file)",
	.add_s=true,
.shortinit=nlshortinit,
.labels=nllabels,
#endif
#ifndef DONTTALK
.checked="aangevinkt",
.unchecked="niet aangevinkt",
.Undetermined="",
.FallingQuickly="Daalt snel",
.Falling="Daalt",
.Stable="Verandert langzaam",
.Rising="Stijgt",
.RisingQuickly="Stijgt snel",
#endif
.receivingpastvalues="Oude glucose waarden komen binnen",
.receivingdata="Gegevens worden ontvangen",
.unsupportedSibionics="Niet-ondersteunde Sibionics-sensor"sv,
.waitingforconnection="Wacht op verbinding"sv,
.deleted="Verwijderd"sv,
.nolocationpermission="Locatie toestemming nodig"sv,
.nonearbydevicespermission="Apparaten in de buurt toestemming nodig"sv

#endif //INJUGGLUCO

,.summarygraph="Samenvattende grafiek"sv
,.logdays="Dagen"sv
,.unhide="Zichtbaar"sv
		}


		;


addlang(nl);

