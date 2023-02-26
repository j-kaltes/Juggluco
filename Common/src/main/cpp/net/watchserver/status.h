const char statusformat[]=R"({
  "status": "ok",
  "name": "nightscout",
  "version": "14.2.6",
  "serverTime": "%d-%02d-%02dT%02d:%02d:%02d.%03dZ",
  "serverTimeEpoch": %ld,
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
    "alarmTimeagoUrgent": true,
    "alarmTimeagoUrgentMins": "30",
    "alarmPumpBatteryLow": false,
    "language": "en",
    "focusHours": 3,
    "authDefaultRoles": "readable",
    "thresholds": {
      "bgHigh": %.1f,
      "bgTargetTop": %.1f,
      "bgTargetBottom": %.1f,
      "bgLow":%.1f 
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
    ]
  },
  "authorized": null,
  "runtimeState": "loaded"
})";
