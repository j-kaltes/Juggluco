#include "jugglucotext.hpp"
#ifdef SPANISH
#ifdef INJUGGLUCO
#ifndef WEAROS
constexpr static std::string_view labels[]={"Carbohidr",
"Dextro",
"Insulina r",
"Insulina l",
"Bici",
"Caminar",
"Sangre"};
constexpr static Shortcut_t  shortinit[]= { {"Pan",
        .48},
        {"Bollo1",
        .56f},
        {"Bollo2",
        .595f},
        {"Uvas",
        .165f},
        {"YogurFrut",
        .058f},
        {"Arroz",
        .75f},
        {"Macarrón",
        .65f},
        {"Tomate",
        .03f},
        {"Mezcla mex",
        .078f},
        {"ZumoNaran",
        .109f},
        {"PolvoDep",
        .873f},
        {"Mix zanah",
        .07f},
        {"Mix champi",
        .07300000f}};
#endif
#endif //INJUGGLUCO

extern const jugglucotext estext;

const jugglucotext estext {
	.daylabel={"Dom", "Lun", "Mar", "Mié", "Jue", "Vie", "Sáb"},
#ifdef INJUGGLUCO
.speakdaylabel={ "domingo","lunes","martes","miércoles","jueves","viernes","sábado"},
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
	.readysecEnable="Sensor listo en %d minutos. Escanéalo de nuevo para activar el flujo continuo.",
	.readysec="Sensor listo en %d minutos.",
.networkproblem="Sin glucosa del clon",
.enablebluetooth="Activa Bluetooth",
.useBluetoothOff="'Usar Bluetooth' desactivado",
.noconnectionerror=": Sin conexión",
.stsensorerror=": Error del sensor",
.streplacesensor=": ¿Sustituir sensor?",
.endedformat="%s dejó de funcionar. estado=%d",
.notreadyformat="%s no está listo. estado=%d",
#ifndef WEAROS
	.median="Mediana",
	.middle="Centro",
#endif //WEAROS
	.history="Historial",
//	.historyinfo="Once per 15 minutes.\nRemembered on the sensor for 8 hours.\nScanning transfers them to this program.\nSensor: ", 
//	.history3info="Once per 5 minutes.\nRemembered on the sensor for 14 days.\nTransferred by Bluetooth to this program.\nSensor: ",
#ifndef WEAROS
	.sensorstarted= "Iniciado:						",
#else
	.sensorstarted= "Iniciado:",
#endif
	.lastscanned="Último escaneo:",
#ifndef WEAROS
	.laststream="Último flujo:		",
#else
	.laststream="Último flujo:",
#endif
	.sensorends="Finaliza oficialmente: ",
	.sensorexpectedend="Fin previsto: ",
#endif //INJUGGLUCO
#ifndef WEAROS

#ifdef INJUGGLUCO
	.newamount="Nueva cantidad",
	.averageglucose="Glucosa media: ",
	.duration="Duración: %.1f días",
	.timeactive="Activo el %.1f%% del tiempo",
	.nrmeasurement="Número de mediciones: %d",
	.EstimatedA1C="A1C estimada: %.1f%% (%d mmol/mol)",
	.GMI="Indicador de gestión de glucosa: %.1f%% (%d mmol/mol)",
	.SD="DE: %.2f",
	.glucose_variability="Variabilidad de la glucosa: %.1f%%",
     .menustr0={
		"Interfaz sist.         ",
		"Menús",
		"Reloj",
		"Sensor",
		"Ajustes",

#if defined(SIBIONICS)
		"Foto",
#else
		"Acerca de",
#endif

		"Cerrar",
		"Detener alarma"
		},
	.menustr1={
		"Exportar",
		"Clon",
		estext.newamount,
		"Lista", 
#else
       .statistics=
#endif //INJUGGLUCO
		"Estadísticas",
#ifdef INJUGGLUCO
		"Hablar",
		"Flotante     "
		},
	.menustr2= {"Último esc.",
	"Escaneos",
	"Flujo",
	"Historial",
	"Cantidades    ",
	"Comidas",
	"Modo oscuro         "},
	.menustr3= {hourminstr,
	"Buscar",
	"Fecha",
	"Día ant.",
	"Día sig.",
	"Sem. ant.",
	"Sem. sig."},
#endif //INJUGGLUCO
#else
 .amount="Cantidad",
 .menustr0= {
	"        Clon",
	"Sensor",
	"Pantalla",
    "Ajustes",
	"Detener alarma" },
.menustr2= {"Fecha ",
hourminstr,
"Día ant.                      ",
estext.amount},
#endif //WEAROS
#ifdef INJUGGLUCO
	.scanerrors={
		{"Error de escaneo (%d)", "Inténtalo de nuevo"},
		{"Error de instalación", "?"},
		{"Error de procesamiento de datos", "Inténtalo de nuevo"},
		{"Activando sensor", ""},
		{"El sensor ha finalizado", ""},
		{"Sensor listo en", "%d minutos"},
		{"Error del sensor (373)", "Inténtalo más tarde"},
		{"Nuevo sensor inicializado", "Escanéalo de nuevo para usarlo"},
		{"", "Bloquea el toque durante el escaneo"},
		{"", ""},
		{"Error al iniciar la biblioteca", "¿Faltan bibliotecas compartidas?"},
		{"Error al iniciar la clase", "Haz algo"},
		{"El procedimiento tarda demasiado", "Cierro el programa"},
		{"Sustituir sensor (365)", "Tu sensor no funciona. Retíralo e inicia uno nuevo."},
		{"Sustituir sensor (368)", "Tu sensor no funciona. Retíralo e inicia uno nuevo."},
		{"", ""},
		{"Error de escaneo", "Inténtalo de nuevo"}},

.libre3scanerror={"FreeStyle Libre 3, error de escaneo", 
	"Inténtalo de nuevo"},
.libre3wrongID={"Error, ¿ID de cuenta incorrecto?",
	R"(Debe ser el mismo que al activar el sensor. Usa Menú izquierdo->Ajustes->Intercambiar datos->Libreview->"Obtener ID de cuenta" para recuperarlo desde Libreview.)"},
.libre3scansuccess= {"Sensor FreeStyle Libre 3", 
	"Juggluco recibirá ahora los valores de glucosa"},
.unknownNFC={"Error de escaneo NFC no reconocido", 
	"Inténtalo de nuevo"},
.nolibre3={"Sensor FreeStyle Libre 3",
	"Descarga la versión correcta desde https://www.juggluco.nl/download.html"},
.libre3zeroID={"Error, ¿ID de cuenta cero?",
	R"(Usa Menú izquierdo->Ajustes->Intercambiar datos->Libreview->"Obtener ID de cuenta" para establecer un ID de cuenta distinto de cero.)"},
.needsandroid8="Requiere como mínimo Android 8"sv,
#ifndef WEAROS
	.advancedstart= R"(<h1>Dispositivo modificado</h1>
<p>Una de las bibliotecas que usa esta aplicación tiene un ERROR que la hace fallar si detecta ciertos archivos. Tu dispositivo contiene algunos de esos archivos. Este programa incluye un ajuste para sortear ese ERROR, pero probablemente sea mejor hacer que esos archivos no se puedan detectar de otra manera. Magisk, por ejemplo, permite ocultar el acceso root para ciertas aplicaciones (Magiskhide o Denylist) y cambiar su propio nombre; en tu caso hacen falta ambas cosas. En tu caso tiene problemas con el siguiente archivo)",
	.add_s=true,
.shortinit=shortinit,
.labels=labels,
#endif
#ifndef DONTTALK
.checked="marcado",
.unchecked="sin marcar",
.Undetermined=""sv,
.FallingQuickly="Bajando rápido"sv,
.Falling="Bajando"sv,
.Stable="Cambiando lentamente"sv,
.Rising="Subiendo"sv,
.RisingQuickly="Subiendo rápido"sv,
#endif
.receivingpastvalues="Recibiendo valores anteriores"sv,
.receivingdata="Recibiendo datos"sv,
.unsupportedSibionics="Sensor Sibionics no compatible"sv,
.waitingforconnection="Esperando conexión"sv,
.deleted="Eliminado"sv,
.nolocationpermission="Necesita permiso de ubicación"sv,
.nonearbydevicespermission="Necesita permiso de dispositivos cercanos"sv,
#endif //INJUGGLUCO
.summarygraph="Gráfico resumido"sv
,.logdays="Registro diario"sv
,.unhide="Mostrar"sv
		}


		;

#include "logs.hpp"
addlang(es);
#endif

