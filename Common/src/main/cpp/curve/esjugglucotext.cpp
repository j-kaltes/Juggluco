#include "config.h"
#ifdef SPANISH
#include "jugglucotext.h"
#ifndef WEAROS
constexpr static std::string_view eslabels[]={"Fast Insuli",
"Carbohidra",
"Dextro",
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

;
#endif

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
.networkproblem="No hay glucosa en el espejo",
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
// .historyinfo="Una vez cada 15 minutos.\nRecordado en el sensor durante 8 horas.\nEl escaneo los transfiere a este programa.\nSensor: ",
// .history3info="Una vez cada 5 minutos.\nRecordado en el sensor durante 14 días.\nTransferido por Bluetooth a este programa.\nSensor: ",
.sensorstarted= "Iniciado:",
.lastscanned="Último escaneo:",
.laststream="Última transmisión:",
.sensorends="Termina:",
#ifndef WEAROS
.newamount="Nueva cantidad",
.averageglucose="Glucosa promedio: ",
.duration="Duración: %.1f días",
.timeactive="%.1f%% del tiempo activo",
.nrmeasurement="Número de mediciones: %d",
.EstimatedA1C="A1C estimada: %.1f%% (%d mmol/mol)",
.GMI="Indicador de Manejo de Glucosa: %.1f%% (%d mmol/mol)",
.SD="SD: %.2f",
.glucose_variability="Variabilidad de la glucosa: %.1f%%",
      .menustr0={
"Interfaz",
"Menús",
"Reloj",
"Sensor",
"Ajustes",
"Acerca",
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
.menustr2= {"Último escaneo",
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
" Espejo",
"Sensor",
"   Oscuro      ",
         "Ajustes",
"Detener alarma" },
.menustr2= {"Fecha ",
hourminstr,
"Día atrás               ",
estext.amount},
#endif

.scanerrors={
{"Error de escaneo (%d)",
"Intentar otra vez"},
{"Error de instalación",
"?"},
{"Error de procesamiento de datos",
"Intentar otra vez"},
{"Activando Sensor",
""},
{"El sensor definitivamente ha terminado",
""},

{"Sensor listo",
"%d minutos"},
{"373: Error del sensor",
"Inténtalo de nuevo más tarde"},
{"Nuevo sensor inicializado",
"Escanear nuevamente para usarlo"},
{"",
"Bloquea contacto durante escaneo"},
{"",
""},
{"Error de inicialización de la biblioteca",
"¿Faltan bibliotecas compartidas?"},
{"Error de inicialización de clase",
"Hacer algo"},
{"El trámite tarda demasiado",
"Cierro el programa"},
{"365: Reemplazar sensor",
"Su sensor no funciona. Retire su sensor y comience uno nuevo."},
{"368: Reemplazar sensor",
"Su sensor no funciona. Retire su sensor y comience uno nuevo."},
{"",
""},
{"Error de exploración",
"Intentar otra vez"}},

.libre3scanerror={"FreeStyle Libre 3, Error de escaneo",
"Intentar otra vez"},
.libre3wrongID={"Error, ¿ID de cuenta incorrecto?",
"Especifique en Configuración->Libreview la misma cuenta utilizada para activar el sensor"},
.libre3scansuccess= {"Sensor FreeStyle Libre 3",
"Jugggluco ahora recibirá los valores de glucosa"},
.unknownNFC={"Error de escaneo NFC no reconocido",
"Intentar otra vez"},
.nolibre3={"Sensor FreeStyle Libre 3",
"No es compatible con esta versión de Juggluco"},
#ifndef WEAROS
.advancedstart= R"(<h1>Dispositivo modificado</h1>
<p>Una de las bibliotecas utilizadas por esta aplicación tiene un ERROR que hace
que se bloquée si detecta ciertos archivos. Su dispositivo contiene algunos de
estos archivos. Este programa contiene un truco para evitar este ERROR, pero
Probablemente sea mejor hacer que estos archivos sean indetectables en algún otro
forma. Magisk, por ejemplo, tiene la opción de ocultar la raíz para ciertos
aplicaciones (Magiskhide o Denylist) y cambiar su propio nombre, tanto
Se necesitan. En su caso tiene problemas con el siguiente archivo)",
.add_s=true,
.shortinit=esshortinit,
.labels=eslabels,
.checked="comprobado",
.unchecked="no marcado",
.Undetermined="",
.FallingQuickly="Disminuyendo rápidamente",
.Falling="Disminuyendo",
.Stable="Cambiando lentamente",
.Rising="Aumentando",
.RisingQuickly="Aumentando rápidamente",
#endif
.receivingpastvalues="Receiving old values"

}


;

#include "logs.h"
extern void setusees() ;
void setusees() {
  LOGAR("switch to ES");
  usedtext= &estext;
  }
#endif
