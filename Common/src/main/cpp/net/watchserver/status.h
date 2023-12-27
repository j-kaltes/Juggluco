#include "cmdline/version.h"
const char statusformat[]=R"({
  "status": "ok",
  "name": "juggluco",
  "version": ")" APPVERSION R"(",
  "serverTime": "%d-%02d-%02dT%02d:%02d:%02d.%03dZ",
  "serverTimeEpoch": %lld,
  "apiEnabled": true,
  "careportalEnabled": false,
  "boluscalcEnabled": false,
  "settings": {
    "units": "%s",
    "timeFormat": 24,
    "editMode": false,
    "showRawbg": "never",
    "customTitle": "Juggluco",
    "theme": "default",
    "alarmUrgentHigh": false,
    "alarmUrgentHighMins": [
      30,
      60,
      90,
      120
    ],
    "alarmHigh": false,
    "alarmHighMins": [
      30,
      60,
      90,
      120
    ],
    "alarmLow": true,
    "alarmLowMins": [
      15,
      30,
      45,
      60
    ],
    "alarmUrgentLow": true,
    "alarmUrgentLowMins": [
      15,
      30,
      45
    ],
    "alarmUrgentMins": [
      30,
      60,
      90,
      120
    ],
    "alarmWarnMins": [
      30,
      60,
      90,
      120
    ],
    "alarmTimeagoWarn": true,
    "alarmTimeagoWarnMins": "15",
    "alarmTimeagoUrgent": false,
    "alarmTimeagoUrgentMins": "30",
    "alarmPumpBatteryLow": false,
    "language": "en",
    "focusHours": 3,
    "authDefaultRoles": "readable",
    "thresholds": {
      "bgHigh": %.0f,
      "bgTargetTop": %.0f,
      "bgTargetBottom": %.0f,
      "bgLow":%.0f 
    },
    "insecureUseHttp": false,
    "deNormalizeDates": false,
    "showClockDelta": false,
    "showClockLastTime": true,
    "authFailDelay": 5000,
    "adminNotifiesEnabled": false,
    "DEFAULT_FEATURES": [
    ],
    "alarmTypes": [
      "simple"
    ],
    "enable": [
    "cors"
    ]
  },
  "authorized": null,
  "runtimeState": "loaded"
})";
