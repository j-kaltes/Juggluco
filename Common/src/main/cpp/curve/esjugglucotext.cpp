#include "config.h"
#ifdef SPANISH
#include "jugglucotext.hpp"
#include "jugglucotext.hpp"
#ifndef WEAROS
constexpr static std::string_view eslabels[]={"Carbohidra",
"Dextro",
"Fast Insuli",
"Insu largos",
"Bicicleta",
"Caminar",
"Sangre"};
constexpr static Shortcut_t esshortinit[]=
{ {"Pan",
        .48},
        {"Currantbun1",
        .56f},
        {"Currantbun2",
        .595f},
        {"Uvas",
        .165f},
        {"FruitYog",
        .058f},
        {"Arroz",
        .75f},
        {"Macarrones",
        .65f},
        {"Tomate",
        .03f},
        {"Mexican mix",
        .078f},
        {"Zumo fruta",
        .109f},
        {"SportPowder",
        .873f},
        {"Mix(Carrot)",
        .07f},
        {"Mix mushro",
        .07300000f}};
#endif

extern jugglucotext estext;

jugglucotext estext {
.daylabel={"Dom",
"Lun",
"Mar",
"Mie",
"Jue",
"Vie",
"Sab"},
.speakdaylabel={ "Domingo","Lunes","Martes","Miércoles","Jueves","Viernes","Sábado"},
.monthlabel={
       "Ene",
       "Feb",
       "Mar",
       "Abr",
       "May"      ,
       "Jun",
        "Jul",
        "Ago",
        "Sep",
       "Oct",
       "Nov",
       "Dic"},


.scanned="Escaneado",
.readysecEnable="Sensor listo en %d minutos. Escanee nuevamente para habilitar la transmisión.",
.readysec="Sensor listo en %d minutos.",
.networkproblem="No hay glucosa del espejo",
.enablebluetooth="Habilitar Bluetooth",
.useBluetoothOff="'Usar Bluetooth' desactivado",
.noconnectionerror=": Sin conexión",
.stsensorerror=": Error del sensor",
.streplacesensor=": ¿Reemplazar el sensor?",
.endedformat="%s dejó de funcionar. estado=%d",
.notreadyformat="%s no está listo. estado=%d",
#ifndef WEAROS
.median="Mediana",
.middle="Media",
#endif
.history="Historial",
// .historyinfo="Una vez cada 15 minutos.\nSe recuerdan en el sensor durante 8 horas.\nAl escanearlos, se transfieren a este programa.\nSensor: ",
// .history3info="Una vez cada 5 minutos.\nSe recuerdan en el sensor durante 14 días.\nSe transfieren por Bluetooth a este programa.\nSensor: ",
.sensorstarted= "Iniciado:",
.lastscanned="Último escaneado:",
.laststream="Última transmisión:",
.sensorends="Finaliza oficialmente: ",
.sensorexpectedend="Finalización prevista: ",
#ifndef WEAROS
.newamount="Nueva cantidad",
.averageglucose="Glucosa promedio: ",
.duration="Duración: %.1f días",
.timeactive="%.1f%% del tiempo activo",
.nrmeasurement="Número de mediciones: %d",
.EstimatedA1C="A1C estimada: %.1f%% (%d mmol/mol)",
.GMI="Indicador de gestión de glucosa: %.1f%% (%d mmol/mol)",
.SD="SD: %.2f",
.glucose_variability="Variabilidad de la glucosa: %.1f%%",
.menustr0={
"Interfaz        ",
"Menús",
"Reloj",
"Sensor",
"Ajustes",
#if defined(SIBIONICS)
"Foto",
#else
"Acerca",
#endif
"Cerrar",
"Detener alarma"
},
.menustr1={
"Exportar",
"Espejo",
estext.newamount,
"Lista",
"Estadísticas",
"Hablar",
"Flotante        "
},
.menustr2= 
    {"Calibrated",
"Escaneos",
"Flujos",
"Historial",
"Cantidades",
"Comidas",
"Modo oscuro         "},

.menustr3= {hourminstr,
"Buscar",
"Fecha",
"Día atrás",
"Día después",
"Semana atrás",
"Semana despues"},
#else
  .amount="Cuantía",
  .menustr0= {
"        Espejo",
"Sensor",
"Display",
         "Ajustes",
"Detener alarma" },
.menustr2= {"Fecha ",
hourminstr,
"Día atrás               ",
estext.amount},
#endif

.scanerrors={
{"Error de escaneo (%d)",
"Intentar de nuevo"},
{"Error de instalación",
"?"},
{"Error de procesamiento de datos",
"Intentar de nuevo"},
{"Activando sensor",
""},
{"El sensor ha finalizado definitivamente",
""},

{"Sensor listo en",
"%d minutos"},
{"Error de sensor (373)",
"Intentar de nuevo más tarde"},
{"Nuevo sensor inicializado",
"Escanear de nuevo para usarlo"},
{"",
"Bloquea el tacto durante el escaneo"},
{"",
""},
{"Error de inicialización de biblioteca",
"¿Faltan bibliotecas compartidas?"},
{"Clase Error de inicialización",
"Haz algo"},
{"El procedimiento tarda demasiado",
"Cierro el programa"},
{"Reemplazar sensor (365)", "Tu sensor no está funcionando. Por favor, retira tu sensor y empieza uno nuevo."},
{"Reemplazar sensor (368)",
"Tu sensor no está funcionando. Retire su sensor y comience uno nuevo."},
{"",
""},
{"Error de escaneo",
"Inténtelo de nuevo"}},

.libre3scanerror={"FreeStyle Libre 3, error de escaneo",
"Inténtelo de nuevo"},
.libre3wrongID={"Error, ¿ID de cuenta incorrecta?",
R"(Debería ser el mismo que cuando se activa el sensor. Use el menú izquierdo->Configuración->Libreview->"Obtener ID de cuenta" para recuperarlo de Libreview.)"},
.libre3scansuccess= {"Sensor FreeStyle Libre 3",
"Juggluco recibirá ahora los valores de glucosa"},
.unknownNFC={"Error de escaneo NFC no reconocido",
"Inténtelo de nuevo"},
.nolibre3={"Sensor FreeStyle Libre 3",
.libre3zeroID={"Error, zero account ID?",
	R"(Use Left menu->Settings->Exchange data->Libreview->"Get Account ID" to set a non-zero account ID.)"},
.needsandroid8="Needs minimally Android 8"sv,
"Descargue la versión correcta de https://www.juggluco.nl/download.html"},
#ifndef WEAROS
.advancedstart= R"(<h1>Dispositivo modificado</h1>
<p>Una de las bibliotecas utilizadas por esta aplicación tiene un ERROR que hace que
se bloquee si detecta ciertos archivos. Su dispositivo contiene algunos de
estos archivos. Este programa contiene un truco para evitar este ERROR, pero
probablemente sea mejor hacer que estos archivos sean indetectables de alguna otra
manera. Magisk, por ejemplo, tiene la opción de ocultar la raíz para ciertas
aplicaciones (Magiskhide o Denylist) ycambiar su propio nombre, ambos
son necesarios. En tu caso tiene problemas con el siguiente archivo)",
.add_s=true,
.shortinit=esshortinit,
.labels=eslabels,
#endif
#ifndef DONTTALK
.checked="checked",
.unchecked="not checked",
.Undetermined=""sv,
.FallingQuickly="Cayendo rápidamente"sv,
.Falling="Cayendo"sv,
.Stable="Cambiando lentamente"sv,
.Rising="Aumentando"sv,
.RisingQuickly="Aumentando rápidamente"sv,
#endif
.receivingpastvalues="Recibiendo valores antiguos"sv,
.receivingdata="Recibiendo datos"sv,
.unsupportedSibionics="Sensor Sibionics no compatible"sv,
.waitingforconnection="Esperando conexión"sv,
.deleted="Deleted"sv,
.nolocationpermission="Needs location permission"sv,
.nonearbydevicespermission="Needs nearby devices permission"sv

,.summarygraph="Summary graph"sv
,.logdays="Daily log"sv
,.unhide="Unhide"sv

}

;


#include "logs.hpp"
addlang(es);
#endif

