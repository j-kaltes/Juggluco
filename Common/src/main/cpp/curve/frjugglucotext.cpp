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
constexpr static std::string_view labels[]={"Ins. rapide","Glucides","Dextro","Ins. basale","Vélo","Marche","Sang"};
constexpr static Shortcut_t  shortinit[]= { {"Pain blanc",
        .48},
        {"Pain gris",
        .56f},
        {"Pâtes",
        .595f},
        {"Raisins",
        .165f},
        {"Yaourt",
        .058f},
        {"Riz",
        .75f},
        {"Macaroni",
        .65f},
        {"Tomates",
        .03f},
        {"Mix Mexica",
        .078f},
        {"Jus oranges",
        .109f},
        {"Mix sport",
        .873f},
        {"Mix carotte",
        .07f},
        {"Aquarius",
        .07300000f}};
#endif

jugglucotext frtext {
	.daylabel={"Dim","Lun","Mar","Mer","Jeu","Ven","Sam"},
//	.monthlabel={ "Janv","Fevr","Mars","Avril","Mai"      ,             "Juin", "Juil","Août","Sept", "Oct","Nov","Déc"},
	.monthlabel={
      "Jan","Fev","Mar","Avr","Mai"      ,             "Jun",
       "Jui","Aou","Sep",
      "Oct","Nov","Déc"},

	.scanned="Scanné",
	.readysecEnable="Capteur prêt dans %d minutes. Scannez à nouveau pour activer l'acquisition.",
	.readysec="Capteur prêt dans %d minutes.",
.networkproblem="Problème réseau ?",
.enablebluetooth="Activer Bluetooth",
.useBluetoothOff="'Utiliser Bluetooth' coupé",
.noconnectionerror=": Pas de connexion.",
.stsensorerror=": Erreur du capteur",
.streplacesensor=": Remplacer le capteur ?",
#ifndef WEAROS
	.median="Moyenne",
	.middle="Milieu",
#endif
	.history="Historique",
	.historyinfo="Une fois toutes les 15 minutes.\nMémorisé dans le capteur durant 8 heures.\nLe scan transfère les mesures dans ce programme.\nCapteur: ",
	.history3info="Une fois toutes les 5 minutes.\nMémorisé dans le capteur durant 14 jours.\nTransféré par Bluetooth dans ce programme.\nCapteur: ",
	.sensorstarted= "Capteur démarré:",
	.lastscanned="Dernier scan:",
	.laststream="Dernier flux:",
	.sensorends="Expiration capteur: ",
#ifndef WEAROS
	.newamount="Nouvelle quantité",
	.averageglucose="Glucose moyen: ",
	.duration="Durée: %.1f jours",
	.timeactive="actif %.1f%% du temps",
	.nrmeasurement="Nombre de mesures: %d",
	.EstimatedA1C="HbA1c estimé: %.1f%% (%d mmol/mol)",
	.GMI="Indicateur de gestion du glucose: %.1f%% (%d mmol/mol)",
	.SD="SD: %.2f",
	.glucose_variability="Variabilité glycémique: %.1f%%",
     	.menustr0={
		"Intf. système        ",
		"Montre",
		"Capteur",
		"Paramètres",
		"A propos",
		"Fermer",
		"Arrêt alarme"
		},
	.menustr1={
		"Exporter",
		"Miroir",
		frtext.newamount,
		"Liste", 
		"Statistiques",
		"Parler",
		"Flotter        "
		},
	.menustr2= {"Dernier scan","Scans","Flux","Historique","Quantités","Repas","Mode sombre        "},
	.menustr3= {hourminstr,"Rechercher","Date","Jour avant","Jour après","Semaine avant","Semaine après"},
#else
 .amount="Quantité",
 .menustr0= {
	"Miroir",
	"Capteur",
	"Modesombre      ",
        "Paramètres",
	"Arrêt alarme" },
.menustr2= {"Date",hourminstr,"Jour avant",frtext.amount},
#endif

	.scanerrors={
		{"Erreur de scan (%d)","Réessayez"},
		{"Erreur d'installation","?"},
		{"Erreur traitement des données","Réessayez"},
		{"Activation du capteur",""},
		{"Le capteur a définitivement expiré",""},

		{"Capteur prêt dans","%d minutes"},
		{"373: Erreur capteur","Ne contactez pas le service clients d'Abott immédiatement; il est probable que les données seron disponibles dans 10 minutes."},
		{"Nouveau capteur initialisé","Scannez à nouveau pour l'utiliser"},
		{"","Bloquer le tactile durant le scan"},
		{"",""},
		{"Erreur d'initialisation de la librairie","Manque-t-il des librairies partagées?"},
		{"Erreur d'initialisation de classe","Faites quelque chose..."},
		{"Processus trop long","Je ferme le programme"},
		{"365: Remplacez le capteur","Votre capteur ne fonctionne pas. Vous devez le remplacer et démarrer un nouveau."},
		{"368: Remplacez le capteur","Votre capteur ne fonctionne pas. Vous devez le remplacer et démarrer un nouveau."},
		{"",""},
		{"Erreur de scan","Réessayez"}},

.libre3scanerror={"FreeStyle Libre 3, Scan error", "Try again"},
.libre3wrongID={"Error, wrong account ID?","Specify in Settings->Libreview the same account used to activate the sensor"},
.libre3scansuccess= {"FreeStyle Libre 3 sensor", "Glucose values will now be received by Juggluco"},
.unknownNFC={"Unrecognized NFC scan Error", "Try again"},
.nolibre3={"FreeStyle Libre 3 sensor","Not supported by this version of Juggluco"},
#ifndef WEAROS
	.advancedstart= R"(<h1>Appareil modifié</h1>
<p>Une des librairies utilisées par cette appli a un bug qui la fait
planter si elle détecte certains fichiers. Votre appareil comprend certains
de ces fichiers. Ce programme contient un correctif contournant ce bug,
mais il vaut sûrement mieux rendre ces fichiers non détectables d'une autre
façon. Magisk, par exemple, a l'option d'effacer root pour certaines
applications (Magiskhide ou Denylist) et changer son propre nom, les deux sont 
requis. Dans votre cas le fichier suivant pose problème )",
	.add_s=true,
.shortinit=shortinit,
.labels=labels
#endif
		}


		;


extern void setusefr() ;
void setusefr() {
	usedtext= &frtext;
	}
