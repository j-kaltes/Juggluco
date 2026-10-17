package tk.glucodata.ui.data

import android.app.Activity
import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import androidx.lifecycle.DefaultLifecycleObserver
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleOwner
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import kotlinx.coroutines.sync.Mutex
import kotlinx.coroutines.withContext
import org.json.JSONArray
import org.json.JSONObject
import tk.glucodata.Applic
import tk.glucodata.BleMirror
import tk.glucodata.BluetoothGlucoseMeter
import tk.glucodata.BuildConfig
import tk.glucodata.Log
import tk.glucodata.MainActivity
import tk.glucodata.MeterBleSupport
import tk.glucodata.GarminBridge
import tk.glucodata.JugglucoSend
import tk.glucodata.MessageSender
import tk.glucodata.Natives
import tk.glucodata.NightPost
import tk.glucodata.Nightscout
import androidx.annotation.StringRes
import tk.glucodata.R
import tk.glucodata.Notify
import tk.glucodata.SensorBridge
import tk.glucodata.SendLikexDrip
import tk.glucodata.SuperGattCallback
import tk.glucodata.WatchBridge
import tk.glucodata.XInfuus
import tk.glucodata.nums.numio
import tk.glucodata.ui.meters.MeterFound
import tk.glucodata.ui.meters.MeterInfo
import tk.glucodata.ui.meters.MeterRuntime
import tk.glucodata.ui.meters.MeterScanState
import tk.glucodata.ui.meters.MeterScanStatus
import tk.glucodata.ui.meters.MeterSettings
import tk.glucodata.ui.model.AgpProfile
import tk.glucodata.ui.model.AlarmBehavior
import tk.glucodata.ui.model.AlarmConfig
import tk.glucodata.ui.model.AlarmSoundStream
import tk.glucodata.ui.model.BroadcastReceiverApp
import tk.glucodata.ui.model.DeltaCalculation
import tk.glucodata.ui.model.DisplayConfig
import tk.glucodata.ui.model.ExchangesConfig
import tk.glucodata.ui.model.GarminLibre3Result
import tk.glucodata.ui.model.GarminShortcut
import tk.glucodata.ui.model.GarminShortcutError
import tk.glucodata.ui.model.GarminStatus
import tk.glucodata.ui.model.GlucosePoint
import tk.glucodata.ui.model.GlucoseRange
import tk.glucodata.ui.model.GlucoseStats
import tk.glucodata.ui.model.GlucoseUnit
import tk.glucodata.ui.model.HardwareConfig
import tk.glucodata.ui.model.InsulinType
import tk.glucodata.ui.model.LabelConfig
import tk.glucodata.ui.model.LabelSaveError
import tk.glucodata.ui.model.LibreLabelMapping
import tk.glucodata.ui.model.LibreRegion
import tk.glucodata.ui.model.LibreSaveError
import tk.glucodata.ui.model.LibreTreatmentKind
import tk.glucodata.ui.model.LibreViewConfig
import tk.glucodata.ui.model.LogRecord
import tk.glucodata.ui.model.LogType
import tk.glucodata.ui.model.MealItem
import tk.glucodata.ui.model.MirrorConnection
import tk.glucodata.ui.model.MirrorConnectionDraft
import tk.glucodata.ui.model.MirrorDataStart
import tk.glucodata.ui.model.MirrorHostEditState
import tk.glucodata.ui.model.MirrorImportPreview
import tk.glucodata.ui.model.MirrorPresentData
import tk.glucodata.ui.model.MirrorQuickCode
import tk.glucodata.ui.model.MirrorSaveError
import tk.glucodata.ui.model.MirrorSaveResult
import tk.glucodata.ui.model.NumberStore
import tk.glucodata.ui.model.NumberStoreSource
import tk.glucodata.ui.model.RangeLevel
import tk.glucodata.ui.model.SensorDetail
import tk.glucodata.ui.model.SensorInfo
import tk.glucodata.ui.model.SensorState
import tk.glucodata.ui.model.SensorStatus
import tk.glucodata.ui.model.SignalQuality
import tk.glucodata.ui.model.StatsPeriod
import tk.glucodata.ui.model.TimeRange
import tk.glucodata.ui.model.TreatmentMapping
import tk.glucodata.ui.model.UploaderConfig
import tk.glucodata.ui.model.UploaderStatus
import tk.glucodata.ui.model.WatchConfig
import tk.glucodata.ui.model.WearDiagnosticInfo
import tk.glucodata.ui.model.WearWatchDevice
import tk.glucodata.ui.sync.DisplaySync
import java.util.concurrent.atomic.AtomicBoolean

sealed interface SensorActivationState {
    data object Idle : SensorActivationState
    data object Waiting : SensorActivationState
    data object Reading : SensorActivationState
    data object Activating : SensorActivationState
    data object AwaitingSecondScan : SensorActivationState
    data object Verifying : SensorActivationState
    data class Success(
        val sensorName: String,
        val endTime: Long,
        val canAddToCalendar: Boolean,
        val sensorTypeName: String? = null,
        val warmupMinutes: Int = 60
    ) : SensorActivationState
    data class Failure(val reason: String? = null) : SensorActivationState
    data object Cancelled : SensorActivationState
}

class GlucoseRepository(
    private val scope: CoroutineScope
) {

    /** Resolves a string resource outside of Compose (the repository has no context of its own). */
    private fun res(@StringRes id: Int): String = Applic.getContext().getString(id)
    private data class NativeEntryIdentity(
        val store: NumberStore,
        val timeSeconds: Long,
        val valueBits: Long,
        val label: Int,
        val mealPointer: Int
    )

    private data class PersistedLogNote(
        val id: Long,
        val store: NumberStore,
        val position: Int,
        val identity: NativeEntryIdentity,
        val note: String
    )

    private val logNoteLock = Any()

    /**
     * Guards the expensive native loads. [refreshAll] and the polling heartbeat both walk the whole
     * sensor store, which allocates tens of megabytes per pass; letting two passes overlap turns a
     * stutter into an out-of-memory death spiral on a watch, so at most one is ever in flight and
     * requests that arrive while it runs are coalesced into a single follow-up pass.
     */
    private val nativeLoadMutex = Mutex()

    /** Set by callers wanting a full reload; consumed by whichever pass holds [nativeLoadMutex]. */
    private val nativeLoadPending = AtomicBoolean(false)

    @Volatile
    private var statsRecalcPending = false

    @Volatile
    private var statsRecalcJob: Job? = null

    /** Fingerprints of the values already published, so identical reloads emit nothing. */
    private var publishedReadingsFingerprint = 0L
    private var publishedSensorsFingerprint = 0L
    private var publishedSensorDetailsFingerprint = 0L
    private var publishedLogsFingerprint = 0L

    private val _currentReading = MutableStateFlow<GlucosePoint?>(null)
    val currentReading: StateFlow<GlucosePoint?> = _currentReading.asStateFlow()

    private val _readings = MutableStateFlow<List<GlucosePoint>>(emptyList())
    val readings: StateFlow<List<GlucosePoint>> = _readings.asStateFlow()

    private val _sensors = MutableStateFlow<List<SensorInfo>>(emptyList())
    val sensors: StateFlow<List<SensorInfo>> = _sensors.asStateFlow()

    private val _mirrorConnections = MutableStateFlow<List<MirrorConnection>>(emptyList())
    val mirrorConnections: StateFlow<List<MirrorConnection>> = _mirrorConnections.asStateFlow()

    private val _sensorDetails = MutableStateFlow<List<SensorDetail>>(emptyList())
    val sensorDetails: StateFlow<List<SensorDetail>> = _sensorDetails.asStateFlow()

    private val _previousSensors = MutableStateFlow<List<SensorDetail>>(emptyList())
    val previousSensors: StateFlow<List<SensorDetail>> = _previousSensors.asStateFlow()

    private val _sensorActivationState = MutableStateFlow<SensorActivationState>(SensorActivationState.Idle)
    val sensorActivationState: StateFlow<SensorActivationState> = _sensorActivationState.asStateFlow()

    private val _logs = MutableStateFlow<List<LogRecord>>(emptyList())
    val logs: StateFlow<List<LogRecord>> = _logs.asStateFlow()

    private val _unit = MutableStateFlow(GlucoseUnit.MG_DL)
    val unit: StateFlow<GlucoseUnit> = _unit.asStateFlow()

    /**
     * The four configurable band edges. Everything that classifies or colours a reading reads
     * this, so changing it in settings updates graphs, statistics, widgets and the watch at once.
     */
    private val _range = MutableStateFlow(GlucoseRange())
    val range: StateFlow<GlucoseRange> = _range.asStateFlow()

    private val _targetLow = MutableStateFlow(GlucoseRange.DEFAULT_LOW)
    val targetLow: StateFlow<Float> = _targetLow.asStateFlow()

    private val _targetHigh = MutableStateFlow(GlucoseRange.DEFAULT_HIGH)
    val targetHigh: StateFlow<Float> = _targetHigh.asStateFlow()

    private val _selectedTimeRange = MutableStateFlow<TimeRange?>(TimeRange.SIX_HOURS)
    val selectedTimeRange: StateFlow<TimeRange?> = _selectedTimeRange.asStateFlow()

    private val _stats = MutableStateFlow(GlucoseStats())
    val stats: StateFlow<GlucoseStats> = _stats.asStateFlow()

    private val _screenStats = MutableStateFlow(GlucoseStats())
    val screenStats: StateFlow<GlucoseStats> = _screenStats.asStateFlow()

    private val _statsPeriod = MutableStateFlow(StatsPeriod.FOURTEEN_DAYS)
    val statsPeriod: StateFlow<StatsPeriod> = _statsPeriod.asStateFlow()

    private val _statsUseHistory = MutableStateFlow(true)
    val statsUseHistory: StateFlow<Boolean> = _statsUseHistory.asStateFlow()

    private val _agpProfile = MutableStateFlow(AgpProfile.calculate(emptyList(), StatsPeriod.FOURTEEN_DAYS))
    val agpProfile: StateFlow<AgpProfile> = _agpProfile.asStateFlow()

    private val _alarms = MutableStateFlow(AlarmConfig())
    val alarms: StateFlow<AlarmConfig> = _alarms.asStateFlow()

    private val _alarmBehavior = MutableStateFlow<List<AlarmBehavior>>(emptyList())
    val alarmBehavior: StateFlow<List<AlarmBehavior>> = _alarmBehavior.asStateFlow()

    private val _voiceAnnounce = MutableStateFlow(false)
    val voiceAnnounce: StateFlow<Boolean> = _voiceAnnounce.asStateFlow()

    private val _speakAlarms = MutableStateFlow(true)
    val speakAlarms: StateFlow<Boolean> = _speakAlarms.asStateFlow()

    private val _exchanges = MutableStateFlow(ExchangesConfig())
    val exchanges: StateFlow<ExchangesConfig> = _exchanges.asStateFlow()

    private val _uploader = MutableStateFlow(UploaderConfig())
    val uploader: StateFlow<UploaderConfig> = _uploader.asStateFlow()

    private val _uploaderStatus = MutableStateFlow(UploaderStatus())
    val uploaderStatus: StateFlow<UploaderStatus> = _uploaderStatus.asStateFlow()

    private val _xdripReceiverApps = MutableStateFlow<List<BroadcastReceiverApp>>(emptyList())
    val xdripReceiverApps: StateFlow<List<BroadcastReceiverApp>> = _xdripReceiverApps.asStateFlow()

    private val _xdripReceiverAppsLoading = MutableStateFlow(false)
    val xdripReceiverAppsLoading: StateFlow<Boolean> = _xdripReceiverAppsLoading.asStateFlow()

    private val _glucodataReceiverApps = MutableStateFlow<List<BroadcastReceiverApp>>(emptyList())
    val glucodataReceiverApps: StateFlow<List<BroadcastReceiverApp>> = _glucodataReceiverApps.asStateFlow()

    private val _glucodataReceiverAppsLoading = MutableStateFlow(false)
    val glucodataReceiverAppsLoading: StateFlow<Boolean> = _glucodataReceiverAppsLoading.asStateFlow()

    private val _displayConfig = MutableStateFlow(DisplayConfig())
    val displayConfig: StateFlow<DisplayConfig> = _displayConfig.asStateFlow()

    private val _bloodLabels = MutableStateFlow<List<String>>(emptyList())
    val bloodLabels: StateFlow<List<String>> = _bloodLabels.asStateFlow()

    private val _labelConfig = MutableStateFlow(LabelConfig())
    /** The native labels, and the label each logbook entry type is saved under. */
    val labelConfig: StateFlow<LabelConfig> = _labelConfig.asStateFlow()

    private val _insulinOnboardEnabled = MutableStateFlow(false)
    /** Whether the insulin on board calculation runs; off until the user asks for it. */
    val insulinOnboardEnabled: StateFlow<Boolean> = _insulinOnboardEnabled.asStateFlow()

    private val _insulinOnboard = MutableStateFlow<Float?>(null)
    /**
     * The insulin still on board right now, in insulin units, or null while the calculation is
     * off or native has nothing to say. Only ever published when
     * [insulinOnboardEnabled] is true, so the glucose screen can leave it out entirely.
     */
    val insulinOnboard: StateFlow<Float?> = _insulinOnboard.asStateFlow()

    private val _hardwareConfig = MutableStateFlow(HardwareConfig())
    val hardwareConfig: StateFlow<HardwareConfig> = _hardwareConfig.asStateFlow()

    private val _metersEnabled = MutableStateFlow(false)
    val metersEnabled: StateFlow<Boolean> = _metersEnabled.asStateFlow()

    private val _meters = MutableStateFlow<List<MeterInfo>>(emptyList())
    val meters: StateFlow<List<MeterInfo>> = _meters.asStateFlow()

    private val _meterRuntimes = MutableStateFlow<List<MeterRuntime>>(emptyList())
    val meterRuntimes: StateFlow<List<MeterRuntime>> = _meterRuntimes.asStateFlow()

    private val _meterScan = MutableStateFlow(MeterScanState())
    val meterScan: StateFlow<MeterScanState> = _meterScan.asStateFlow()

    private val _watchConfig = MutableStateFlow(WatchConfig())
    val watchConfig: StateFlow<WatchConfig> = _watchConfig.asStateFlow()

    private val _wearDevices = MutableStateFlow<List<WearWatchDevice>>(emptyList())
    val wearDevices: StateFlow<List<WearWatchDevice>> = _wearDevices.asStateFlow()

    private val _wearDiagnosticInfo = MutableStateFlow(WearDiagnosticInfo())
    val wearDiagnosticInfo: StateFlow<WearDiagnosticInfo> = _wearDiagnosticInfo.asStateFlow()

    private val _garminStatus = MutableStateFlow(GarminStatus())
    val garminStatus: StateFlow<GarminStatus> = _garminStatus.asStateFlow()

    private val _garminShortcuts = MutableStateFlow<List<GarminShortcut>>(emptyList())
    val garminShortcuts: StateFlow<List<GarminShortcut>> = _garminShortcuts.asStateFlow()

    private val _libreView = MutableStateFlow(LibreViewConfig())
    val libreView: StateFlow<LibreViewConfig> = _libreView.asStateFlow()

    private val _libreTreatments = MutableStateFlow<List<LibreLabelMapping>>(emptyList())
    val libreTreatments: StateFlow<List<LibreLabelMapping>> = _libreTreatments.asStateFlow()

    /**
     * Whether "send amounts" may be switched on: either it already is, or every label has been
     * given a treatment kind. Read from native so it keeps agreeing with the uploader.
     */
    private val _libreAmountsAllowed = MutableStateFlow(false)
    val libreAmountsAllowed: StateFlow<Boolean> = _libreAmountsAllowed.asStateFlow()

    /**
     * Whether a screen showing this repository is in the foreground. Only the UI reads these flows
     * (alerts, complications and widgets go to native themselves), so the heartbeat sleeps while
     * this is false. A watch keeps the activity alive behind the watch face for days, and polling
     * every few seconds plus a full native reload every two minutes for a screen nobody sees was
     * a steady drain on its battery.
     */
    private val uiVisible = MutableStateFlow(true)

    /** False once the scope it was built on is gone, e.g. the activity owning it was destroyed. */
    val isAlive: Boolean get() = scope.isActive

    /** Runs the heartbeat only while [lifecycle] is at least started. */
    fun followVisibility(lifecycle: Lifecycle) {
        lifecycle.addObserver(object : DefaultLifecycleObserver {
            override fun onStart(owner: LifecycleOwner) = setUiVisible(true)
            override fun onStop(owner: LifecycleOwner) = setUiVisible(false)
        })
    }

    private fun setUiVisible(visible: Boolean) {
        if (uiVisible.value == visible) return
        uiVisible.value = visible
        // The heartbeat only appends the latest reading, so whatever arrived while hidden comes
        // back with one full reload.
        if (visible) refreshAll()
    }

    init {
        DisplaySync.install(this)
        // Never touch JNI or SharedPreferences from whatever thread constructs the repository:
        // on Wear that is the main thread, during activity creation.
        scope.launch(Dispatchers.IO) {
            refreshSettings()
            refreshAllLocked()
            refreshWearDevices()
            refreshMirrorConnections()
            startPolling()
        }
    }

    fun setTimeRange(range: TimeRange?) {
        _selectedTimeRange.value = range
        requestStatsRecalculation()
    }

    fun setStatsPeriod(period: StatsPeriod) {
        _statsPeriod.value = period
        requestStatsRecalculation()
    }

    fun setStatsUseHistory(useHistory: Boolean) {
        _statsUseHistory.value = useHistory
        try {
            if (Applic.Nativesloaded) {
                Natives.analysedays(_statsPeriod.value.days, useHistory)
            }
        } catch (_: Throwable) {}
        requestStatsRecalculation()
    }

    fun setUnit(newUnit: GlucoseUnit) {
        _unit.value = newUnit
        val nativeUnit = if (newUnit == GlucoseUnit.MMOL_L) 1 else 2
        try {
            if (Applic.Nativesloaded) {
                if (Applic.app != null) {
                    Applic.app.setunit(nativeUnit)
                } else {
                    Natives.setunit(nativeUnit)
                    Applic.unit = nativeUnit
                }
            }
        } catch (_: Throwable) {
            try {
                if (Applic.Nativesloaded) Natives.setunit(nativeUnit)
            } catch (_: Throwable) {}
            Applic.unit = nativeUnit
        }
    }

    /**
     * The single write path for the four band edges. Values are sanitized before anything sees
     * them, persisted to `settings.dat` for the legacy view and the native statistics, and pushed
     * into every held reading so the graph, logbook and hero recolour immediately.
     */
    fun setGlucoseRange(newRange: GlucoseRange) {
        val sanitized = newRange.normalized()
        _range.value = sanitized
        _targetLow.value = sanitized.lowMgDl
        _targetHigh.value = sanitized.highMgDl
        try {
            if (Applic.Nativesloaded) {
                val unit = _unit.value
                Natives.setTargetRange(unit.toDisplay(sanitized.lowMgDl), unit.toDisplay(sanitized.highMgDl))
                Natives.setVeryRange(unit.toDisplay(sanitized.veryLowMgDl), unit.toDisplay(sanitized.veryHighMgDl))
            }
        } catch (_: Throwable) {}
        reclassifyReadings()
        requestStatsRecalculation()
    }

    /** Moves one band edge, keeping the four cut points ordered and separated. */
    fun setRangeLevel(level: RangeLevel, valueMgDl: Float) {
        setGlucoseRange(_range.value.withLevel(level, valueMgDl))
    }

    fun canSelectBloodLabel(index: Int): Boolean {
        val config = _labelConfig.value
        return config.label(index)?.name?.isNotBlank() == true &&
            config.roleOf(index, except = LogType.BLOOD_GLUCOSE) == null
    }

    /**
     * Native calibration only looks at the blood label, so one that is not set falls back to the
     * "Blood" label of the native defaults, as long as that one is not the meal label.
     */
    private fun ensureBloodLabel() {
        try {
            val names = Natives.getLabels().orEmpty().dropLast(1)
            val configured = Natives.getbloodvar().toInt()
            if (names.getOrNull(configured)?.isNotBlank() != true &&
                names.getOrNull(DEFAULT_BLOOD_LABEL)?.isNotBlank() == true &&
                Natives.getmealvar().toInt() != DEFAULT_BLOOD_LABEL
            ) {
                Natives.setbloodvar(DEFAULT_BLOOD_LABEL.toByte())
            }
        } catch (_: Throwable) {}
    }

    /** Re-reads the labels from native and publishes them. */
    private fun reloadLabels(): LabelConfig {
        if (!Applic.Nativesloaded) return _labelConfig.value
        if (LabelDefaults.apply(::labelInUse)) sendLabels()
        ensureBloodLabel()
        val config = NativeLabels.read()
        _labelConfig.value = config
        _bloodLabels.value = config.labels.map { it.name }
        if (_displayConfig.value.bloodLabelIndex != config.bloodLabel) {
            _displayConfig.value = _displayConfig.value.copy(bloodLabelIndex = config.bloodLabel)
        }
        return config
    }

    /** Whether anything in the logbook was ever saved under native label [label]. */
    private fun labelInUse(label: Int): Boolean {
        for (store in NumberStore.values()) {
            try {
                val ptr = numberStorePointer(store) ?: continue
                for (pos in Natives.getfirstNum(ptr) until Natives.getlastNum(ptr)) {
                    val itm = Natives.getNumitem(ptr, pos) ?: continue
                    if (itm.time > 0 && itm.label == label) return true
                }
            } catch (_: Throwable) {
                // Unreadable counts as used: renaming a label is only safe on proof it is empty.
                return true
            }
        }
        return false
    }

    /** The labels as currently configured, reading them first if that never happened yet. */
    private fun currentLabels(): LabelConfig =
        _labelConfig.value.takeIf { it.labels.isNotEmpty() } ?: reloadLabels()

    /** Reloads the labels and the logbook, for a change made elsewhere (classic view, the phone). */
    fun refreshLabels() {
        scope.launch(Dispatchers.IO) {
            reloadLabels()
            loadLogsFromNative()
        }
    }

    fun refreshSettings() {
        try {
            if (Applic.Nativesloaded) {
                val nativeUnit = Natives.getunit()
                val unit = GlucoseUnit.fromNative(nativeUnit)
                _unit.value = unit
                if (Applic.app != null && Applic.unit != nativeUnit) {
                    Applic.app.setunit(nativeUnit)
                } else {
                    Applic.unit = nativeUnit
                }
                val storedRange = GlucoseRange(
                    veryLowMgDl = unit.toMgDl(Natives.verylow()),
                    lowMgDl = unit.toMgDl(Natives.targetlow()),
                    highMgDl = unit.toMgDl(Natives.targethigh()),
                    veryHighMgDl = unit.toMgDl(Natives.veryhigh())
                ).normalized()
                if (storedRange != _range.value) {
                    _range.value = storedRange
                    _targetLow.value = storedRange.lowMgDl
                    _targetHigh.value = storedRange.highMgDl
                    // The range can also be changed outside this process (the legacy settings
                    // screen, a restored backup), so the held readings have to catch up here too.
                    reclassifyReadings()
                    requestStatsRecalculation()
                }

                // Read Alarms (native getters return display-unit values, see
                // settings.hpp gconvert/tomgperL, so fallbacks are unit-aware)
                val mmol = _unit.value == GlucoseUnit.MMOL_L
                _alarms.value = AlarmConfig(
                    lowAlarmEnabled = Natives.hasalarmlow(),
                    lowThreshold = Natives.alarmlow().let { if (it > 0f) it else if (mmol) 3.9f else 70f },
                    lowSnoozeMinutes = Natives.readalarmsuspension(0).toInt().coerceAtLeast(5),
                    highAlarmEnabled = Natives.hasalarmhigh(),
                    highThreshold = Natives.alarmhigh().let { if (it > 0f) it else if (mmol) 10f else 180f },
                    highSnoozeMinutes = Natives.readalarmsuspension(1).toInt().coerceAtLeast(5),
                    urgentLowEnabled = try { Natives.hasalarmverylow() } catch (_: Throwable) { true },
                    urgentLowThreshold = try { Natives.alarmverylow().let { if (it > 0f) it else if (mmol) 3f else 54f } } catch (_: Throwable) { if (mmol) 3f else 54f },
                    urgentLowSnoozeMinutes = try { Natives.readalarmsuspension(5).toInt().coerceAtLeast(5) } catch (_: Throwable) { 15 },
                    veryHighEnabled = try { Natives.hasalarmveryhigh() } catch (_: Throwable) { false },
                    veryHighThreshold = try { Natives.alarmveryhigh().let { if (it > 0f) it else if (mmol) 13.9f else 250f } } catch (_: Throwable) { if (mmol) 13.9f else 250f },
                    veryHighSnoozeMinutes = try { Natives.readalarmsuspension(6).toInt().coerceAtLeast(5) } catch (_: Throwable) { 30 },
                    preLowEnabled = try { Natives.hasalarmprelow() } catch (_: Throwable) { false },
                    preLowThreshold = try { Natives.alarmprelow().let { if (it > 0f) it else if (mmol) 4.4f else 80f } } catch (_: Throwable) { if (mmol) 4.4f else 80f },
                    preLowSnoozeMinutes = try { Natives.readalarmsuspension(7).toInt().coerceAtLeast(5) } catch (_: Throwable) { 15 },
                    preHighEnabled = try { Natives.hasalarmprehigh() } catch (_: Throwable) { false },
                    preHighThreshold = try { Natives.alarmprehigh().let { if (it > 0f) it else if (mmol) 9.4f else 170f } } catch (_: Throwable) { if (mmol) 9.4f else 170f },
                    preHighSnoozeMinutes = try { Natives.readalarmsuspension(8).toInt().coerceAtLeast(5) } catch (_: Throwable) { 15 },
                    lossAlarmEnabled = Natives.hasalarmloss(),
                    lossWaitMinutes = Natives.readalarmsuspension(4).toInt().coerceAtLeast(10),
                    valueAvailableNotification = Natives.hasvaluealarm(),
                    soundStream = AlarmSoundStream.fromId(Natives.getalarmSoundType())
                )

                // Read Exchanges
                val xdripReceiverPackages = Natives.xdripRecepters()
                    .filterNotNull()
                    .map { it.trim() }
                    .filter { it.isNotEmpty() }
                    .distinct()
                val glucodataReceiverPackages = Natives.glucodataRecepters()
                    .filterNotNull()
                    .map { it.trim() }
                    .filter { it.isNotEmpty() }
                    .distinct()
                _exchanges.value = ExchangesConfig(
                    xdripBroadcast = xdripReceiverPackages.isNotEmpty(),
                    xdripReceiverPackages = xdripReceiverPackages,
                    glucodataBroadcast = glucodataReceiverPackages.isNotEmpty(),
                    glucodataReceiverPackages = glucodataReceiverPackages,
                    librelinkBroadcast = Natives.getlibrelinkused(),
                    everSenseBroadcast = try { Natives.geteverSensebroadcast() } catch (_: Throwable) { false },
                    healthConnect = try { Natives.gethealthConnect() } catch (_: Throwable) { false },
                    libreViewEnabled = Natives.getuselibreview(),
                    xdripWebServer = Natives.getusexdripwebserver()
                )

                // Read Nightscout uploader
                _uploader.value = UploaderConfig(
                    url = try { Natives.getnightuploadurl().orEmpty() } catch (_: Throwable) { "" },
                    active = try { Natives.getuseuploader() } catch (_: Throwable) { false },
                    v3 = try { Natives.getnightscoutV3() } catch (_: Throwable) { false },
                    postTreatments = try { Natives.getpostTreatments() } catch (_: Throwable) { false },
                    canSendTreatments = try { Natives.canSendNumbers(1) } catch (_: Throwable) { false }
                )
                refreshUploaderStatus()

                val savedMinimalistUnits = try {
                    Applic.app.getSharedPreferences(UI_PREFS, Context.MODE_PRIVATE)
                        .getBoolean(KEY_MINIMALIST_UNITS, true)
                } catch (_: Throwable) { true }
                val savedDeltaMinutes = try {
                    Applic.app.getSharedPreferences(UI_PREFS, Context.MODE_PRIVATE)
                        .getInt(KEY_DELTA_CALCULATION, 1)
                } catch (_: Throwable) { 1 }
                val savedShowAlertLines = try {
                    Applic.app.getSharedPreferences(UI_PREFS, Context.MODE_PRIVATE)
                        .getBoolean(KEY_SHOW_ALERT_LINES, false)
                } catch (_: Throwable) { false }
                val deltaCalculation = DeltaCalculation.fromMinutes(savedDeltaMinutes)
                val resolvedBloodLabel = reloadLabels().bloodLabel

                // Read Display
                _displayConfig.value = DisplayConfig(
                    floatingGlucose = Natives.getfloatglucose(),
                    statusBarNotification = Natives.getshowalways(),
                    // Native systemUI stores whether the bars are shown. The Compose
                    // setting expresses the inverse: whether immersive fullscreen is on.
                    systemUiFullscreen = !Natives.getsystemUI(),
                    invertColors = Natives.getInvertColors(),
                    showScans = Natives.getshowscans(),
                    showCalibratedScans = Natives.getshowcalibratedscans(),
                    showStream = Natives.getshowstream(),
                    showCalibratedStream = Natives.getshowcalibratedstream(),
                    showHistory = Natives.getshowhistories(),
                    showCalibratedHistory = Natives.getshowcalibratedhistories(),
                    showAmounts = Natives.getshownumbers(),
                    showMeals = Natives.getshowmeals(),
                    showAlertLines = savedShowAlertLines,
                    minimalistUnits = savedMinimalistUnits,
                    deltaCalculation = deltaCalculation,
                    calibrationEnabled = try { Natives.getDoCalibrate() } catch (_: Throwable) { false },
                    bloodLabelIndex = resolvedBloodLabel,
                    calibratePastReadings = try { Natives.getCalibratePast() } catch (_: Throwable) { false },
                    calibrateAllValues = try { Natives.getAllValues() } catch (_: Throwable) { false },
                    use24Hour = try { Natives.gethour24() } catch (_: Throwable) { true }
                )

                // Read Hardware
                _hardwareConfig.value = HardwareConfig(
                    nfcSound = Natives.nfcsound(),
                    globalScanStartsApp = isNfcLaunchEnabled(),
                    googleScan = try { Natives.getGoogleScan() } catch (_: Throwable) { false },
                    hasNfc = MainActivity.hasnfc
                )

                // Read the glucose meter feature, which is only used when someone switched it on.
                _metersEnabled.value = MeterSettings.isEnabled(Applic.app)
                if (_metersEnabled.value) readMetersNow()

                // Read Watch & Wear OS
                _watchConfig.value = WatchConfig(
                    wearOsEnabled = WatchBridge.isWearOsEnabled(),
                    garminEnabled = try { Natives.getusegarmin() } catch (_: Throwable) { false },
                    watchdripEnabled = try { Natives.getwatchdrip() } catch (_: Throwable) { false },
                    gadgetbridgeEnabled = try { SuperGattCallback.doGadgetbridge } catch (_: Throwable) { false },
                    separateAlerts = try { Notify.alertseparate } catch (_: Throwable) { false },
                    notifyWatch = WatchBridge.getNotifyWatch()
                )

                // Read per-alarm sound behavior, voice output and NFC sound
                _alarmBehavior.value = readAlarmBehavior()
                _voiceAnnounce.value = try { Natives.getVoiceActive() } catch (_: Throwable) { false }
                _speakAlarms.value = try { Natives.speakalarms() } catch (_: Throwable) { true }

                // Read LibreView
                readLibreViewState()

                // Read the insulin on board flag and its value, which live in native settings
                publishInsulinOnboard()
            }
        } catch (_: Throwable) {}
    }

    /**
     * Reloads the LibreView account, upload and treatment-mapping state from native. The mapping
     * is one kind lookup and one weight lookup per label, and the weight is only needed for labels
     * that map to carbs, so this stays cheap enough to run in the settings pass.
     */
    private fun readLibreViewState() {
        val manualAccountId = try { Natives.manualLibreAccountIDnumber() != -1L } catch (_: Throwable) { false }
        _libreView.value = LibreViewConfig(
            region = LibreRegion.fromNative(
                try { Natives.getLibreCountry() } catch (_: Throwable) { LibreRegion.UNITED_KINGDOM.nativeIndex }
            ),
            accountId = try { Natives.getlibreAccountIDnumber() } catch (_: Throwable) { -1L },
            hasAccountId = manualAccountId ||
                (try { !Natives.getlibreAccountID().isNullOrBlank() } catch (_: Throwable) { false }),
            manualAccountId = manualAccountId,
            uploadCurrent = try { Natives.getLibreCurrent() } catch (_: Throwable) { false },
            uploadViewed = try { Natives.getLibreIsViewed() } catch (_: Throwable) { false },
            sendAmounts = try { Natives.getSendNumbers() } catch (_: Throwable) { false }
        )
        _libreTreatments.value = readLibreTreatments()
        _libreAmountsAllowed.value = try { Natives.canSendNumbers(LIBRE_NIGHT) } catch (_: Throwable) { false }
    }

    private fun readLibreTreatments(): List<LibreLabelMapping> {
        val labels = try { Natives.getLabels().toList() } catch (_: Throwable) { return emptyList() }
        // The last entry is native's empty "no label", which is never mapped to a treatment.
        return labels.dropLast(1).mapIndexed { index, short ->
            val label = LabelNames.fullName(index, short.orEmpty())
            val kind = LibreTreatmentKind.fromNative(
                try { Natives.getlibrenumkind(LIBRE_NIGHT, index) } catch (_: Throwable) { 0 }
            )
            LibreLabelMapping(
                index = index,
                label = label,
                kind = kind,
                weight = if (kind == LibreTreatmentKind.CARBS) {
                    try { Natives.getlibrefoodweight(LIBRE_NIGHT, index) } catch (_: Throwable) { 1f }
                } else {
                    1f
                }
            )
        }
    }

    fun refreshLibreView() {
        scope.launch(Dispatchers.IO) { readLibreViewState() }
    }

    fun refreshAll() {
        scope.launch(Dispatchers.IO) { refreshAllLocked() }
    }

    /**
     * Reloads everything, but never concurrently with itself. A reload that arrives while another is
     * running simply marks the work as still-pending and returns; the pass already in flight picks
     * the flag up and runs exactly once more on the way out. A burst of BLE callbacks therefore
     * collapses into one extra pass instead of N overlapping walks of the sensor store.
     */
    private suspend fun refreshAllLocked() {
        nativeLoadPending.set(true)
        if (!nativeLoadMutex.tryLock()) return
        try {
            while (nativeLoadPending.getAndSet(false)) {
                refreshSettings()
                loadReadingsFromNative()
                loadSensorsFromNative()
                loadLogsFromNative()
                recalculateStats()
            }
        } finally {
            nativeLoadMutex.unlock()
        }
    }

    /**
     * Coalesces stats recomputation onto a single background pass. This used to run inline on the
     * caller, which meant five full passes over every reading plus an AGP profile build (24 boxing
     * buckets, a [java.util.Calendar] lookup per reading and two sorts) on the main thread of a UI
     * that was only trying to react to a target-range tap.
     */
    private fun requestStatsRecalculation() {
        statsRecalcPending = true
        if (statsRecalcJob?.isActive == true) return
        statsRecalcJob = scope.launch(Dispatchers.Default) {
            while (statsRecalcPending) {
                statsRecalcPending = false
                recalculateStats()
            }
        }
    }

    /**
     * Re-stamps the status of every held reading with the current range. Readings carry their
     * status so collectors don't have to re-derive it, which means a range change would otherwise
     * leave the graph markers, logbook rows and hero coloured against the old cut points until the
     * next full reload. Runs off the main thread; the list can hold six figures of points.
     */
    private fun reclassifyReadings() {
        val range = _range.value
        val current = _readings.value
        val currentReading = _currentReading.value
        scope.launch(Dispatchers.Default) {
            var changed = false
            val updated = ArrayList<GlucosePoint>(current.size)
            for (point in current) {
                val status = range.statusOf(point.valueMgDl)
                if (status == point.status) {
                    updated.add(point)
                } else {
                    changed = true
                    updated.add(point.copy(status = status))
                }
            }
            if (changed) publishReadings(updated)
            if (currentReading != null) {
                val status = range.statusOf(currentReading.valueMgDl)
                if (status != currentReading.status) {
                    _currentReading.value = currentReading.copy(status = status)
                }
            }
        }
    }

    fun beginSensorActivation() {
        _sensorActivationState.value = SensorActivationState.Waiting
    }

    fun reportSensorTagRead() {
        if (_sensorActivationState.value is SensorActivationState.Waiting) {
            _sensorActivationState.value = SensorActivationState.Reading
        }
    }

    fun reportSensorActivationCommand(success: Boolean) {
        if (!success) {
            reportSensorActivationFailure()
        } else if (_sensorActivationState.value in setOf(
                SensorActivationState.Reading,
                SensorActivationState.Activating
            )
        ) {
            _sensorActivationState.value = SensorActivationState.AwaitingSecondScan
        }
    }

    fun reportSensorActivated(sensorName: String) {
        if (_sensorActivationState.value !in setOf(
                SensorActivationState.Waiting,
                SensorActivationState.Reading,
                SensorActivationState.Activating,
                SensorActivationState.AwaitingSecondScan
            )
        ) {
            return
        }
        _sensorActivationState.value = SensorActivationState.Verifying
        scope.launch(Dispatchers.IO) {
            loadSensorsFromNative()
            val endData = try {
                if (Applic.Nativesloaded) Natives.getSensorEndData(sensorName) else 0L
            } catch (_: Throwable) {
                0L
            }
            val endTime = (endData and 0xFFFFFFFFL) * 1000L
            if (_sensorActivationState.value is SensorActivationState.Verifying) {
                val detail = _sensorDetails.value.firstOrNull { it.id == sensorName }
                _sensorActivationState.value = SensorActivationState.Success(
                    sensorName = sensorName,
                    endTime = endTime,
                    canAddToCalendar = (endData ushr 32) != 0L && endTime > System.currentTimeMillis(),
                    sensorTypeName = detail?.sensorTypeName,
                    warmupMinutes = detail?.warmupMinutes ?: 60
                )
            }
        }
    }

    fun reportSensorActivationFailure(reason: String? = null) {
        if (_sensorActivationState.value !in setOf(
                SensorActivationState.Waiting,
                SensorActivationState.Reading,
                SensorActivationState.Activating,
                SensorActivationState.AwaitingSecondScan,
                SensorActivationState.Verifying
            )
        ) {
            return
        }
        _sensorActivationState.value = SensorActivationState.Failure(reason)
    }

    fun cancelSensorActivation() {
        if (_sensorActivationState.value is SensorActivationState.Success) return
        _sensorActivationState.value = SensorActivationState.Cancelled
    }

    fun resetSensorActivation() {
        _sensorActivationState.value = SensorActivationState.Idle
    }

    /**
     * Publishes [next] only when it differs from what every collector already has.
     *
     * A reload always allocates a brand new list, and a `StateFlow` conflates on `equals`, so
     * assigning unconditionally meant every poll produced a structural comparison of the whole
     * history and - when anything at all had shifted - a recomposition of the home screen and a full
     * redraw of the graph. Scanning for a change first costs one linear pass over primitives and
     * emits nothing in the overwhelmingly common case where the sensor store has not moved.
     */
    private fun publishReadings(next: List<GlucosePoint>) {
        val fingerprint = readingsFingerprint(next)
        if (fingerprint == publishedReadingsFingerprint && _readings.value.size == next.size) return
        publishedReadingsFingerprint = fingerprint
        _readings.value = next
    }

    /**
     * Order-sensitive 64-bit digest of the history. Two lists with the same fingerprint are treated
     * as identical, so a false "unchanged" would only ever cost a skipped redraw, never wrong data
     * at a scale where a collision is conceivable.
     */
    private fun readingsFingerprint(readings: List<GlucosePoint>): Long {
        var hash = 1125899906842597L
        for (i in readings.indices) {
            val point = readings[i]
            hash = hash * 31 + point.timestamp
            hash = hash * 31 + point.valueMgDl.toRawBits()
            hash = hash * 31 + (if (point.isCalibrated) 1L else 0L)
            hash = hash * 31 + (if (point.isScan) 1L else 0L)
            hash = hash * 31 + point.sensorIndex
            // Status is part of the identity: reclassifying after a range change has to be
            // publishable, or the graph markers and logbook keep the old colours.
            hash = hash * 31 + point.status.ordinal.toLong()
        }
        return hash * 31 + readings.size
    }

    private fun loadReadingsFromNative() {
        val loadedList = ArrayList<GlucosePoint>(INITIAL_READING_CAPACITY)
        // Hoisted out of the read loops: a StateFlow read per point, over six figures of points, is
        // not free and the value cannot change halfway through a single pass.
        val range = _range.value
        try {
            if (Applic.Nativesloaded) {
                // Check latest reading first
                val strGl = Natives.lastglucose()
                if (strGl != null && strGl.time > 0) {
                    val rawVal = try {
                        strGl.value.replace(',', '.').toFloat()
                    } catch (_: Throwable) {
                        0f
                    }
                    val valMgDl = if (_unit.value == GlucoseUnit.MMOL_L) {
                        GlucoseUnit.MMOL_L.toMgDl(rawVal)
                    } else rawVal

                    if (valMgDl > 0f) {
                        _currentReading.value = GlucosePoint(
                            timestamp = strGl.time * 1000L,
                            valueMgDl = valMgDl,
                            rate = strGl.rate,
                            status = range.statusOf(valMgDl)
                        )
                    } else {
                        _currentReading.value = null
                    }
                } else {
                    _currentReading.value = null
                }

                // Read stream points from all sensors (falling back to active)
                val allPtrs = try { Natives.allSensorPtrs() } catch (_: Throwable) { null }
                val sensorPtrs: LongArray = if (allPtrs != null && allPtrs.isNotEmpty()) allPtrs else (Natives.activeSensorPtrs() ?: LongArray(0))

                if (sensorPtrs.isNotEmpty()) {
                    for (ptr in sensorPtrs) {
                        if (ptr == 0L) continue
                        val sensorIndex = try { Natives.getSensorIndexFromSensorptr(ptr) } catch (_: Throwable) { 0 }

                        // 1. Raw Stream Readings
                        var pos = 0
                        var safetyLimit = 50000
                        while (safetyLimit-- > 0) {
                            val res = Natives.streamfromSensorptr(ptr, pos)
                            val time = res and 0xFFFFFFFFL
                            val nextPos = (res ushr 48).toInt() and 0xFFFF
                            if (time == 0L || nextPos <= pos) break
                            val mgdL = (res ushr 32).toInt() and 0xFFFF
                            if (mgdL in 20..600) {
                                loadedList.add(
                                    GlucosePoint(
                                        timestamp = time * 1000L,
                                        valueMgDl = mgdL.toFloat(),
                                        isScan = false,
                                        isHistory = false,
                                        isCalibrated = false,
                                        sensorIndex = sensorIndex,
                                        status = range.statusOf(mgdL.toFloat())
                                    )
                                )
                            }
                            pos = nextPos
                        }

                        // 2. Calibrated Stream Readings
                        pos = 0
                        safetyLimit = 50000
                        while (safetyLimit-- > 0) {
                            val res = try { Natives.calibratedStreamfromSensorptr(ptr, pos) } catch (_: Throwable) { 0L }
                            val time = res and 0xFFFFFFFFL
                            val nextPos = (res ushr 48).toInt() and 0xFFFF
                            if (time == 0L || nextPos <= pos) break
                            val mgdL = (res ushr 32).toInt() and 0xFFFF
                            if (mgdL in 20..600) {
                                loadedList.add(
                                    GlucosePoint(
                                        timestamp = time * 1000L,
                                        valueMgDl = mgdL.toFloat(),
                                        isScan = false,
                                        isHistory = false,
                                        isCalibrated = true,
                                        sensorIndex = sensorIndex,
                                        status = range.statusOf(mgdL.toFloat())
                                    )
                                )
                            }
                            pos = nextPos
                        }

                        // 3. Scan Readings
                        pos = 0
                        safetyLimit = 10000
                        while (safetyLimit-- > 0) {
                            val res = try { Natives.scanfromSensorptr(ptr, pos) } catch (_: Throwable) { 0L }
                            val time = res and 0xFFFFFFFFL
                            val nextPos = (res ushr 48).toInt() and 0xFFFF
                            if (time == 0L || nextPos <= pos) break
                            val mgdL = (res ushr 32).toInt() and 0xFFFF
                            if (mgdL in 20..600) {
                                loadedList.add(
                                    GlucosePoint(
                                        timestamp = time * 1000L,
                                        valueMgDl = mgdL.toFloat(),
                                        isScan = true,
                                        isHistory = false,
                                        isCalibrated = false,
                                        sensorIndex = sensorIndex,
                                        status = range.statusOf(mgdL.toFloat())
                                    )
                                )
                            }
                            pos = nextPos
                        }

                        pos = 0
                        safetyLimit = 10000
                        while (safetyLimit-- > 0) {
                            val res = try { Natives.calibratedScanfromSensorptr(ptr, pos) } catch (_: Throwable) { 0L }
                            val time = res and 0xFFFFFFFFL
                            val nextPos = (res ushr 48).toInt() and 0xFFFF
                            if (time == 0L || nextPos <= pos) break
                            val mgdL = (res ushr 32).toInt() and 0xFFFF
                            if (mgdL in 20..600) {
                                loadedList.add(
                                    GlucosePoint(
                                        timestamp = time * 1000L,
                                        valueMgDl = mgdL.toFloat(),
                                        isScan = true,
                                        isHistory = false,
                                        isCalibrated = true,
                                        sensorIndex = sensorIndex,
                                        status = range.statusOf(mgdL.toFloat())
                                    )
                                )
                            }
                            pos = nextPos
                        }
                    }
                }
            }
        } catch (_: Throwable) {}

        if (_currentReading.value == null && loadedList.isNotEmpty()) {
            var latest = loadedList[loadedList.size - 1]
            for (point in loadedList) {
                if (!point.isScan && !point.isCalibrated) latest = point
            }
            _currentReading.value = latest
        }

        // In-place stable sort on the timestamp only. `sortBy` would copy the whole list again and
        // box every timestamp to build a sort key; the readings arrive per sensor and per kind, so
        // they are grouped rather than ordered and do need sorting, but they do not need a second
        // six-figure list to do it.
        loadedList.sortWith { a, b -> a.timestamp.compareTo(b.timestamp) }
        publishReadings(loadedList)
    }

    private fun loadSensorsFromNative() {
        val detailsList = ArrayList<SensorDetail>()
        val legacyList = ArrayList<SensorInfo>()
        val mirroredSensorSource = hasLiveReceiverMirror()
        // Derived from data rather than from the clock. `System.currentTimeMillis()` here made
        // every reload produce a different list, so the sensor list was guaranteed to be unequal and
        // the sensors screen recomposed on every single sweep even when nothing had changed.
        val lastKnownReading = _readings.value.lastOrNull()?.timestamp ?: 0L

        try {
            if (Applic.Nativesloaded) {
                // 1. Query live active GATT sensors via SensorBridge
                val rawList = SensorBridge.getActiveGattSensors()
                if (!rawList.isNullOrEmpty()) {
                    for (info in rawList) {
                        val typeName = when (info.sensorgen) {
                            1 -> "FreeStyle Libre 1 / 2"
                            2 -> "FreeStyle Libre 2"
                            3 -> "FreeStyle Libre 3"
                            0x10 -> "SiBionics (GS1 / GS3)"
                            0x30, 0x40 -> "Dexcom G7 / ONE+"
                            0x50 -> "Accu-Chek SmartGuide"
                            else -> res(R.string.cgm_sensor)
                        }

                        val status = when {
                            !info.isConnected -> SensorStatus.DISCONNECTED
                            info.isStreaming -> SensorStatus.CONNECTED_STREAMING
                            else -> SensorStatus.CONNECTED_IDLE
                        }

                        var start = info.startTime
                        if (start <= 0L && info.dataptr != 0L) {
                            try {
                                start = Natives.getSensorStartmsec(info.dataptr)
                            } catch (_: Throwable) {}
                        }
                        var end = 0L
                        if (!info.serial.isNullOrEmpty()) {
                            try {
                                val endData = Natives.getSensorEndData(info.serial)
                                val expectedSec = endData and 0xFFFFFFFFL
                                if (expectedSec > 0L) {
                                    end = expectedSec * 1000L
                                }
                            } catch (_: Throwable) {}
                        }
                        if (end <= 0L && start > 0L) {
                            end = start + 14 * 24 * 3600 * 1000L
                        }

                        detailsList.add(
                            SensorDetail(
                                id = info.serial ?: res(R.string.sensor_id_unknown),
                                name = info.serial ?: typeName,
                                sensorPtr = info.sensorptr,
                                status = status,
                                sensorGen = info.sensorgen,
                                sensorTypeName = typeName,
                                macAddress = info.macAddress,
                                rssi = info.rssi,
                                signalQuality = if (info.isConnected && info.rssi != null && info.rssi != 0) SignalQuality.fromRssi(info.rssi) else SignalQuality.LOST,
                                startTime = start,
                                endTime = end,
                                lastReadingTime = if (info.isConnected) lastKnownReading else 0L,
                                warmupMinutes = info.warmupMinutes,
                                isConnected = info.isConnected,
                                isStreaming = info.isStreaming,
                                isHidden = info.isHidden,
                                hasCalibration = info.hasCalibration,
                                batteryPercent = null,
                                connectionStatusStr = info.statusStr ?: res(
                                    if (info.isConnected) R.string.sensor_handshake_connected
                                    else R.string.sensor_handshake_disconnected
                                ),
                                handshakeStatusStr = info.handshakeStr ?: "",
                                rawDiagnosticText = info.infoHtml ?: "",
                                isPaused = info.isPaused
                            )
                        )
                        legacyList.add(
                            SensorInfo(
                                id = info.serial ?: res(R.string.sensor_id_unknown),
                                name = info.serial ?: typeName,
                                state = if (info.isConnected) SensorState.ACTIVE else SensorState.DISCONNECTED,
                                startTime = start,
                                endTime = end,
                                lastReadingTime = if (info.isConnected) lastKnownReading else 0L,
                                sensorType = typeName,
                                isStreaming = info.isStreaming,
                                isConnected = info.isConnected
                            )
                        )
                    }
                }

                // 2. Check activeSensorPtrs if no Gatt callbacks found
                if (detailsList.isEmpty()) {
                    val ptrs = Natives.activeSensorPtrs()
                    if (ptrs != null && ptrs.isNotEmpty()) {
                        for (ptr in ptrs) {
                            if (ptr == 0L) continue
                            val name = Natives.namefromSensorptr(ptr) ?: res(R.string.sensor_name_generic)
                            val infoText = Natives.sensortextfromSensorptr(ptr) ?: ""
                            val warmup = SensorBridge.warmupMinutes(ptr)
                            val isHidden = try { Natives.getHidefromSensorptr(ptr) } catch (_: Throwable) { false }
                            val hasCali = try { Natives.calibrateNR(ptr, 0) > 0 || Natives.calibrateNR(ptr, 1) > 0 } catch (_: Throwable) { false }

                            var end = 0L
                            try {
                                val endData = Natives.getSensorEndData(name)
                                val expectedSec = endData and 0xFFFFFFFFL
                                if (expectedSec > 0L) {
                                    end = expectedSec * 1000L
                                }
                            } catch (_: Throwable) {}
                            // The real start time, never derived from the end time: sensors are 7, 10,
                            // 14, 15 and 22 days long, so a "end minus 14 days" guess puts a fresh
                            // 15 day sensor in the future and made the warmup countdown read ~1450 min.
                            val start = try {
                                val secs = Natives.getSensorStartSecs(ptr)
                                if (secs > 0L) secs * 1000L else 0L
                            } catch (_: Throwable) {
                                0L
                            }

                            detailsList.add(
                                SensorDetail(
                                    id = name,
                                    name = name,
                                    sensorPtr = ptr,
                                    status = SensorStatus.CONNECTED_STREAMING,
                                    sensorGen = 3,
                                    sensorTypeName = "FreeStyle Libre 3",
                                    macAddress = null,
                                    rssi = null,
                                    signalQuality = SignalQuality.GOOD,
                                    startTime = start,
                                    endTime = end,
                                    lastReadingTime = lastKnownReading,
                                    warmupMinutes = warmup,
                                    isConnected = true,
                                    isStreaming = true,
                                    isMirrored = mirroredSensorSource,
                                    isHidden = isHidden,
                                    hasCalibration = hasCali,
                                    batteryPercent = null,
                                    connectionStatusStr = res(R.string.sensor_handshake_connected),
                                    handshakeStatusStr = res(R.string.sensor_handshake_authenticated),
                                    handshakeStatusRes = R.string.sensor_handshake_authenticated,
                                    rawDiagnosticText = infoText,
                                    // No GATT callback here, but the pause flag is global, so a
                                    // sensor paused before it lost its callback still reads as paused.
                                    isPaused = SensorBridge.isPaused(name)
                                )
                            )
                            legacyList.add(
                                SensorInfo(
                                    id = name,
                                    name = name,
                                    state = SensorState.ACTIVE,
                                    startTime = start,
                                    endTime = end,
                                    lastReadingTime = lastKnownReading,
                                    sensorType = if (infoText.isNotEmpty()) infoText else res(R.string.sensor_name_active),
                                    isConnected = true,
                                    isStreaming = true
                                )
                            )
                        }
                    }
                }
            }
        } catch (_: Throwable) {}

        publishSensors(legacyList, detailsList)
    }

    /** Publishes the sensor lists only when their contents actually differ from the last publish. */
    private fun publishSensors(legacy: List<SensorInfo>, details: List<SensorDetail>) {
        val legacyHash = legacyFingerprint(legacy)
        val detailsHash = sensorDetailFingerprint(details)
        if (legacyHash != publishedSensorsFingerprint) {
            publishedSensorsFingerprint = legacyHash
            _sensors.value = legacy
        }
        if (detailsHash != publishedSensorDetailsFingerprint) {
            publishedSensorDetailsFingerprint = detailsHash
            _sensorDetails.value = details
        }
        val activeIds = details.mapTo(HashSet(details.size)) { it.id }
        _previousSensors.value = _previousSensors.value.filterNot { it.id in activeIds }
    }

    private fun legacyFingerprint(sensors: List<SensorInfo>): Long {
        var hash = 17L
        for (sensor in sensors) {
            hash = hash * 31 + sensor.id.hashCode()
            hash = hash * 31 + sensor.state.hashCode()
            hash = hash * 31 + sensor.startTime
            hash = hash * 31 + sensor.endTime
            hash = hash * 31 + sensor.lastReadingTime
            hash = hash * 31 + (if (sensor.isConnected) 1L else 0L)
            hash = hash * 31 + (if (sensor.isStreaming) 1L else 0L)
        }
        return hash * 31 + sensors.size
    }

    private fun sensorDetailFingerprint(details: List<SensorDetail>): Long {
        var hash = 19L
        for (sensor in details) {
            hash = hash * 31 + sensor.id.hashCode()
            hash = hash * 31 + sensor.status.hashCode()
            hash = hash * 31 + sensor.signalQuality.hashCode()
            hash = hash * 31 + sensor.startTime
            hash = hash * 31 + sensor.endTime
            hash = hash * 31 + sensor.lastReadingTime
            hash = hash * 31 + (sensor.rssi ?: 0)
            hash = hash * 31 + (if (sensor.isConnected) 1L else 0L)
            hash = hash * 31 + (if (sensor.isStreaming) 1L else 0L)
            hash = hash * 31 + (if (sensor.isMirrored) 1L else 0L)
            hash = hash * 31 + (if (sensor.isPaused) 1L else 0L)
        }
        return hash * 31 + details.size
    }

    private fun hasLiveReceiverMirror(): Boolean {
        if (!Applic.Nativesloaded) return false
        try {
            Applic.ensureNetStarted()
        } catch (_: Throwable) {}
        val hostCount = try { Natives.backuphostNr() } catch (_: Throwable) { 0 }
        for (index in 0 until hostCount) {
            val receiveMode = try { Natives.getbackuphostreceive(index) } catch (_: Throwable) { 0 }
            if ((receiveMode and 2) == 0) continue
            val deactivated = try { Natives.getHostDeactivated(index) } catch (_: Throwable) { false }
            if (deactivated) continue
            val status = try { Natives.mirrorStatus(index) ?: "" } catch (_: Throwable) { "" }
            if (isLiveMirrorStatus(status)) return true
        }
        return false
    }

    private fun isLiveMirrorStatus(status: String): Boolean {
        val normalized = status.replace(MARKUP_PATTERN, "")
        return (normalized.contains("TCP/IP live socket: true", ignoreCase = true) &&
            normalized.contains("receive=true", ignoreCase = true)) ||
            normalized.contains("Direct Bluetooth (BLE GATT)=true", ignoreCase = true) ||
            normalized.contains("Messages (Wear OS MessageClient)=true", ignoreCase = true)
    }

    private fun numberStorePointer(store: NumberStore): Long? {
        val index = store.nativeIndex
        if (index < 0 || index >= numio.numptrs.size) return null
        return numio.numptrs[index].takeIf { it != 0L }
    }

    private fun nativeLabel(type: LogType): Int = currentLabels().labelFor(type)

    private fun sameSource(first: LogRecord, second: LogRecord): Boolean {
        val firstSource = first.nativeSource
        val secondSource = second.nativeSource
        return firstSource != null && firstSource == secondSource
    }

    private fun matchesNativeEntry(
        item: tk.glucodata.nums.item,
        entry: LogRecord
    ): Boolean {
        return item.time == entry.timestamp / 1000L &&
            item.value == entry.value &&
            item.label == (entry.nativeLabel ?: nativeLabel(entry.type))
    }

    private fun syncNumberStore(store: NumberStore) {
        if (!Applic.isWearable) {
            try {
                Applic.app?.numdata?.changedback(store.nativeIndex)
            } catch (_: Throwable) {}
        }
    }

    private fun logNotePreferences() = Applic.app?.getSharedPreferences(LOG_NOTES_PREFS, Context.MODE_PRIVATE)

    private fun readPersistedLogNotes(): MutableList<PersistedLogNote> {
        val serialized = logNotePreferences()?.getString(LOG_NOTES_KEY, null) ?: return mutableListOf()
        val notes = mutableListOf<PersistedLogNote>()
        try {
            val array = JSONArray(serialized)
            for (index in 0 until array.length()) {
                val item = array.optJSONObject(index) ?: continue
                val store = NumberStore.entries.firstOrNull { it.nativeIndex == item.optInt("store") } ?: continue
                notes += PersistedLogNote(
                    id = item.optLong("id"),
                    store = store,
                    position = item.optInt("position"),
                    identity = NativeEntryIdentity(
                        store = store,
                        timeSeconds = item.optLong("timeSeconds"),
                        valueBits = item.optLong("valueBits") and 0xffffffffL,
                        label = item.optInt("label"),
                        mealPointer = item.optInt("mealPointer")
                    ),
                    note = item.optString("note")
                )
            }
        } catch (_: Throwable) {}
        return notes
    }

    private fun writePersistedLogNotes(notes: List<PersistedLogNote>) {
        try {
            val array = JSONArray()
            notes.forEach { item ->
                array.put(JSONObject().apply {
                    put("id", item.id)
                    put("store", item.store.nativeIndex)
                    put("position", item.position)
                    put("timeSeconds", item.identity.timeSeconds)
                    put("valueBits", item.identity.valueBits)
                    put("label", item.identity.label)
                    put("mealPointer", item.identity.mealPointer)
                    put("note", item.note)
                })
            }
            logNotePreferences()?.edit()?.putString(LOG_NOTES_KEY, array.toString())?.commit()
        } catch (_: Throwable) {}
    }

    private fun nativeIdentity(item: tk.glucodata.nums.item, store: NumberStore): NativeEntryIdentity {
        return NativeEntryIdentity(
            store = store,
            timeSeconds = item.time,
            valueBits = java.lang.Float.floatToRawIntBits(item.value).toLong() and 0xffffffffL,
            label = item.label,
            mealPointer = item.mealptr
        )
    }

    private fun nativeIdentity(record: LogRecord): NativeEntryIdentity {
        return NativeEntryIdentity(
            store = record.nativeSource?.store ?: NumberStore.HERE,
            timeSeconds = record.timestamp / 1000L,
            valueBits = java.lang.Float.floatToRawIntBits(record.value).toLong() and 0xffffffffL,
            label = record.nativeLabel ?: nativeLabel(record.type),
            mealPointer = record.mealPointer
        )
    }

    private fun sourceKey(source: NumberStoreSource): String = "${source.store.nativeIndex}:${source.position}"

    private fun findNativeSource(
        store: NumberStore,
        identity: NativeEntryIdentity,
        preferredPosition: Int
    ): NumberStoreSource? {
        val ptr = numberStorePointer(store) ?: return null
        return try {
            var found: NumberStoreSource? = null
            for (position in Natives.getfirstNum(ptr) until Natives.getlastNum(ptr)) {
                val item = Natives.getNumitem(ptr, position) ?: continue
                if (nativeIdentity(item, store) != identity) continue
                val candidate = NumberStoreSource(store, position)
                if (position == preferredPosition) return candidate
                if (found == null) found = candidate
            }
            found
        } catch (_: Throwable) {
            null
        }
    }

    private fun upsertPersistedLogNote(note: PersistedLogNote) {
        synchronized(logNoteLock) {
            val notes = readPersistedLogNotes().toMutableList()
            notes.removeAll { it.id == note.id }
            if (note.note.isNotBlank()) notes += note
            writePersistedLogNotes(notes)
        }
    }

    private fun removePersistedLogNote(id: Long) {
        synchronized(logNoteLock) {
            val notes = readPersistedLogNotes()
            if (notes.removeAll { it.id == id }) writePersistedLogNotes(notes)
        }
    }

    private fun reconcilePersistedLogNotes(
        records: List<LogRecord>,
        availableStores: Set<NumberStore>
    ): Map<String, PersistedLogNote> {
        synchronized(logNoteLock) {
            val stored = readPersistedLogNotes()
            val retained = stored.filter { it.store !in availableStores }
            val candidates = records.filter { it.nativeSource?.store in availableStores }
            val usedSources = mutableSetOf<String>()
            val resolved = retained.toMutableList()

            stored.filter { it.store in availableStores }.forEach { persisted ->
                val matches = candidates.filter { record ->
                    val source = record.nativeSource ?: return@filter false
                    source.store == persisted.store &&
                        nativeIdentity(record) == persisted.identity &&
                        sourceKey(source) !in usedSources
                }
                val match = matches.firstOrNull { it.nativeSource?.position == persisted.position } ?: matches.firstOrNull()
                val source = match?.nativeSource ?: return@forEach
                usedSources += sourceKey(source)
                resolved += persisted.copy(
                    store = source.store,
                    position = source.position,
                    identity = persisted.identity
                )
            }

            if (resolved != stored) writePersistedLogNotes(resolved)
            return resolved.mapNotNull { note ->
                note.position.takeIf { it >= 0 }?.let { position ->
                    sourceKey(NumberStoreSource(note.store, position)) to note
                }
            }.toMap()
        }
    }

    private fun loadLogsFromNative() {
        synchronized(logNoteLock) {
            val logList = ArrayList<LogRecord>()
            val availableStores = mutableSetOf<NumberStore>()
            val labels = currentLabels()
            if (Applic.Nativesloaded) {
                for (store in NumberStore.values()) {
                    try {
                        val ptr = numberStorePointer(store) ?: continue
                        val resolvedStore = NumberStore.values().firstOrNull {
                            it.nativeIndex == Natives.getNumindex(ptr)
                        } ?: continue
                        val first = Natives.getfirstNum(ptr)
                        val last = Natives.getlastNum(ptr)
                        availableStores += resolvedStore
                        for (pos in first until last) {
                            val itm = Natives.getNumitem(ptr, pos) ?: continue
                            if (itm.time <= 0) continue
                            val type = labels.typeOf(itm.label)
                            logList.add(
                                LogRecord(
                                    id = LogRecord.nativeId(resolvedStore, pos),
                                    timestamp = itm.time * 1000L,
                                    type = type,
                                    value = itm.value,
                                    nativeSource = NumberStoreSource(resolvedStore, pos),
                                    nativeLabel = itm.label,
                                    mealPointer = itm.mealptr,
                                    labelName = labels.nameOf(itm.label),
                                    mealSummary = if (type == LogType.CARBS && itm.mealptr > 0 && MealStore.available) {
                                        MealStore.summary(itm.mealptr)
                                    } else ""
                                )
                            )
                        }
                    } catch (_: Throwable) {}
                }
            }

            val persistedNotes = reconcilePersistedLogNotes(logList, availableStores)
            val hydratedLogs = logList.map { record ->
                val source = record.nativeSource ?: return@map record
                val persisted = persistedNotes[sourceKey(source)] ?: return@map record
                record.copy(id = persisted.id, note = persisted.note)
            }.toMutableList()
            hydratedLogs.sortWith(
                compareByDescending<LogRecord> { it.timestamp }
                    .thenByDescending { it.nativeSource?.store?.nativeIndex ?: -1 }
                    .thenByDescending { it.nativeSource?.position ?: -1 }
            )
            publishLogs(hydratedLogs)
            // A logged dose is what moves the insulin on board, and this is where every logbook
            // change ends up, so the value follows one reload too.
            publishInsulinOnboard()
        }
    }

    /**
     * Publishes the logbook only when it differs from the last publish. A native reload used to
     * mint a brand new id for every entry, so the list could never compare equal and every collector
     * recomposed every time the logbook was refreshed.
     */
    private fun publishLogs(next: List<LogRecord>) {
        val fingerprint = logFingerprint(next)
        if (fingerprint == publishedLogsFingerprint) return
        publishedLogsFingerprint = fingerprint
        _logs.value = next
    }

    private fun logFingerprint(records: List<LogRecord>): Long {
        var hash = 23L
        for (record in records) {
            hash = hash * 31 + record.timestamp
            hash = hash * 31 + record.value.toRawBits()
            hash = hash * 31 + record.type.hashCode()
            hash = hash * 31 + record.note.hashCode()
            hash = hash * 31 + (record.nativeLabel ?: -1)
            hash = hash * 31 + record.labelName.hashCode()
            hash = hash * 31 + record.mealPointer
            hash = hash * 31 + record.mealSummary.hashCode()
            hash = hash * 31 + (record.nativeSource?.store?.nativeIndex ?: -1)
            hash = hash * 31 + (record.nativeSource?.position ?: -1)
        }
        return hash * 31 + records.size
    }

    fun addCalibrationReference(
        valueMgDl: Float,
        note: String,
        timestamp: Long = System.currentTimeMillis()
    ): Boolean {
        return addLogEntry(LogType.BLOOD_GLUCOSE, valueMgDl, note, timestamp)
    }

    /**
     * Logs an entry of [type], saved under the label configured for it, or under [label] for a
     * [LogType.CUSTOM] entry. A carbs entry can carry the items of a [meal], which are written to
     * the native meal store and attached to it.
     */
    fun addLogEntry(
        type: LogType,
        value: Float,
        note: String,
        timestamp: Long = System.currentTimeMillis(),
        label: Int = -1,
        meal: List<MealItem> = emptyList()
    ): Boolean {
        val labels = currentLabels()
        val targetNativeLabel = if (type == LogType.CUSTOM) {
            label.takeIf { it in labels.labels.indices } ?: -1
        } else {
            labels.labelFor(type)
        }
        if (targetNativeLabel < 0) return false
        val entry = LogRecord(
            timestamp = timestamp,
            type = type,
            value = value,
            note = note,
            nativeLabel = targetNativeLabel,
            labelName = labels.nameOf(targetNativeLabel)
        )
        publishLogs((listOf(entry) + _logs.value).sortedByDescending { it.timestamp })

        scope.launch(Dispatchers.IO) {
            try {
                if (Applic.Nativesloaded) {
                    val store = NumberStore.HERE
                    val ptr = numberStorePointer(store)
                    if (ptr != null) {
                        synchronized(logNoteLock) {
                            val persistedNote = note.trim()
                            // The blood label is what native calibration reads, so it is taken
                            // as it is right now rather than as it was when the editor opened.
                            val labelToSave = if (type == LogType.BLOOD_GLUCOSE) reloadLabels().bloodLabel else targetNativeLabel
                            if (labelToSave < 0) return@synchronized
                            val mealPointer = if (type == LogType.CARBS) MealStore.createMeal(meal) else 0
                            val identity = NativeEntryIdentity(
                                store = store,
                                timeSeconds = timestamp / 1000L,
                                valueBits = java.lang.Float.floatToRawIntBits(value).toLong() and 0xffffffffL,
                                label = labelToSave,
                                mealPointer = mealPointer
                            )
                            if (persistedNote.isNotEmpty()) {
                                upsertPersistedLogNote(
                                    PersistedLogNote(entry.id, store, -1, identity, persistedNote)
                                )
                            }
                            val position = Natives.saveNum(ptr, timestamp / 1000L, value, labelToSave, mealPointer)
                            if (position >= 0 && persistedNote.isNotEmpty()) {
                                val notes = readPersistedLogNotes().toMutableList()
                                notes.replaceAll {
                                    if (it.store == store && it.position >= position) it.copy(position = it.position + 1) else it
                                }
                                notes.removeAll { it.id == entry.id }
                                notes += PersistedLogNote(entry.id, store, position, identity, persistedNote)
                                writePersistedLogNotes(notes)
                            }
                            syncNumberStore(store)
                            loadLogsFromNative()
                        }
                    }
                }
            } catch (_: Throwable) {}
        }
        return true
    }

    /**
     * Changes a logged entry. [meal] replaces the items of the meal attached to a carbs entry;
     * null leaves the meal as it is.
     */
    fun updateLogEntry(
        entry: LogRecord,
        type: LogType,
        value: Float,
        timestamp: Long = entry.timestamp,
        note: String = entry.note,
        meal: List<MealItem>? = null
    ) {
        val targetNativeLabel = if (type == entry.type && entry.nativeLabel != null) {
            entry.nativeLabel
        } else if (type == LogType.BLOOD_GLUCOSE) {
            reloadLabels().bloodLabel
        } else {
            nativeLabel(type)
        }
        if (targetNativeLabel < 0) return
        val source = entry.nativeSource
        if (source == null) {
            publishLogs(_logs.value.map {
                if (it.id == entry.id) it.copy(
                    type = type,
                    value = value,
                    timestamp = timestamp,
                    note = note,
                    nativeLabel = targetNativeLabel
                ) else it
            })
            return
        }

        publishLogs(_logs.value.map {
            if (sameSource(it, entry)) it.copy(
                type = type,
                value = value,
                timestamp = timestamp,
                note = note,
                nativeLabel = targetNativeLabel
            ) else it
        })
        scope.launch(Dispatchers.IO) {
            synchronized(logNoteLock) {
                try {
                    if (Applic.Nativesloaded) {
                        val ptr = numberStorePointer(source.store)
                        val itm = ptr?.let { Natives.getNumitem(it, source.position) }
                        if (ptr != null && itm != null && matchesNativeEntry(itm, entry)) {
                            val hitPtr = Natives.mkhitptr(ptr, source.position)
                            if (hitPtr != 0L) {
                                val ownsMeal = entry.type == LogType.CARBS && source.store == NumberStore.HERE
                                val mealPointer = if (meal != null && type == LogType.CARBS && ownsMeal) {
                                    MealStore.createMeal(meal)
                                } else {
                                    entry.mealPointer
                                }
                                try {
                                    Natives.hitchange(
                                        hitPtr,
                                        timestamp / 1000L,
                                        value,
                                        targetNativeLabel,
                                        mealPointer
                                    )
                                } finally {
                                    Natives.freehitptr(hitPtr)
                                }
                                if (ownsMeal && mealPointer != entry.mealPointer) MealStore.deleteMeal(entry.mealPointer)
                                val newIdentity = NativeEntryIdentity(
                                    store = source.store,
                                    timeSeconds = timestamp / 1000L,
                                    valueBits = java.lang.Float.floatToRawIntBits(value).toLong() and 0xffffffffL,
                                    label = targetNativeLabel,
                                    mealPointer = mealPointer
                                )
                                val newSource = findNativeSource(source.store, newIdentity, source.position)
                                if (newSource != null) {
                                    if (note.isNotBlank()) {
                                        upsertPersistedLogNote(
                                            PersistedLogNote(entry.id, source.store, newSource.position, newIdentity, note.trim())
                                        )
                                    } else {
                                        removePersistedLogNote(entry.id)
                                    }
                                }
                                syncNumberStore(source.store)
                            }
                        }
                        loadLogsFromNative()
                    }
                } catch (_: Throwable) {}
            }
        }
    }

    fun deleteLogEntry(entry: LogRecord) {
        val source = entry.nativeSource
        if (source == null) {
            publishLogs(_logs.value.filterNot { it.id == entry.id })
            return
        }

        publishLogs(_logs.value.filterNot { sameSource(it, entry) })
        scope.launch(Dispatchers.IO) {
            synchronized(logNoteLock) {
                try {
                    if (Applic.Nativesloaded) {
                        val ptr = numberStorePointer(source.store)
                        val itm = ptr?.let { Natives.getNumitem(it, source.position) }
                        if (ptr != null && itm != null && matchesNativeEntry(itm, entry)) {
                            Natives.removeNum(ptr, source.position)
                            if (entry.type == LogType.CARBS && source.store == NumberStore.HERE) {
                                MealStore.deleteMeal(entry.mealPointer)
                            }
                            removePersistedLogNote(entry.id)
                            syncNumberStore(source.store)
                        }
                        loadLogsFromNative()
                    }
                } catch (_: Throwable) {}
            }
        }
    }

    // --- SENSOR ACTIONS (UX Overhaul) ---

    fun setSensorHidden(sensorPtr: Long, hidden: Boolean) {
        _sensorDetails.value = _sensorDetails.value.map {
            if (it.sensorPtr == sensorPtr || sensorPtr == 0L) it.copy(isHidden = hidden) else it
        }
        scope.launch(Dispatchers.IO) {
            try {
                if (Applic.Nativesloaded && sensorPtr != 0L) {
                    Natives.setHidefromSensorptr(sensorPtr, hidden)
                }
            } catch (_: Throwable) {}
        }
    }

    fun useSensorAgain(sensorPtr: Long, activity: Activity?) {
        scope.launch(Dispatchers.IO) {
            try {
                if (Applic.Nativesloaded && sensorPtr != 0L) {
                    SensorBridge.useAgain(activity as? MainActivity, sensorPtr)
                }
            } catch (_: Throwable) {}
            loadSensorsFromNative()
        }
    }

    fun forgetAndRescan(sensorId: String) {
        scope.launch(Dispatchers.IO) {
            try {
                SensorBridge.forgetDevice(sensorId)
            } catch (_: Throwable) {}
            loadSensorsFromNative()
        }
    }

    /**
     * Temporarily disconnects [sensor]: the native sensor session is kept, so reconnecting
     * later restores the same session rather than scanning or activating anything new.
     * Mirrored sensors have no local Bluetooth link, so there is nothing to pause.
     */
    fun pauseSensor(sensor: SensorDetail) {
        scope.launch(Dispatchers.IO) {
            val paused = try {
                Applic.Nativesloaded && SensorBridge.pauseSensor(sensor.id)
            } catch (_: Throwable) {
                false
            }
            if (paused) {
                _sensorDetails.value = _sensorDetails.value.map {
                    if (it.id == sensor.id) it.copy(isPaused = true) else it
                }
            }
            loadSensorsFromNative()
        }
    }

    /** Reconnects a paused sensor to the same native sensor session. */
    fun resumeSensor(sensor: SensorDetail) {
        scope.launch(Dispatchers.IO) {
            try {
                if (Applic.Nativesloaded) {
                    SensorBridge.resumeSensor(sensor.id)
                }
            } catch (_: Throwable) {}
            loadSensorsFromNative()
        }
    }

    fun endSensorPermanently(sensor: SensorDetail) {
        scope.launch(Dispatchers.IO) {
            val ended = try {
                Applic.Nativesloaded && SensorBridge.finishSensor(sensor.id, sensor.sensorPtr)
            } catch (_: Throwable) {
                false
            }
            // A finished sensor is no longer paused, otherwise the flag would outlive the
            // session it was set for and apply to the next sensor with the same serial.
            if (ended) {
                SensorBridge.clearPaused(sensor.id)
            }
            loadSensorsFromNative()
            if (ended) {
                val previous = sensor.copy(
                    status = SensorStatus.ENDED,
                    isConnected = false,
                    isStreaming = false,
                    isPaused = false,
                    rssi = null,
                    signalQuality = SignalQuality.LOST
                )
                _previousSensors.value = listOf(previous) + _previousSensors.value.filterNot { it.id == sensor.id }
            }
        }
    }

    fun resetSensor(sensorPtr: Long) {
        scope.launch(Dispatchers.IO) {
            try {
                if (Applic.Nativesloaded && sensorPtr != 0L) {
                    // SiBionics reset if supported
                }
            } catch (_: Throwable) {}
            loadSensorsFromNative()
        }
    }

    // --- ALARM CONFIG ACTIONS ---

    fun updateAlarms(config: AlarmConfig) {
        _alarms.value = config
        scope.launch(Dispatchers.IO) {
            try {
                if (Applic.Nativesloaded) {
                    Natives.setalarms(
                        config.lowThreshold,
                        config.highThreshold,
                        config.lowAlarmEnabled,
                        config.highAlarmEnabled,
                        config.valueAvailableNotification,
                        config.lossAlarmEnabled
                    )
                    Natives.writealarmsuspension(0, config.lowSnoozeMinutes.toShort())
                    Natives.writealarmsuspension(1, config.highSnoozeMinutes.toShort())
                    Natives.writealarmsuspension(4, config.lossWaitMinutes.toShort())
                    Natives.setalarmSoundType(config.soundStream.id)
                    // Advanced alarms are written as a group; values not edited by
                    // the caller are the ones previously read, so this is a no-op for them.
                    Natives.setAdvancedAlarms(
                        config.urgentLowThreshold,
                        config.veryHighThreshold,
                        config.urgentLowEnabled,
                        config.veryHighEnabled,
                        config.preLowEnabled,
                        config.preHighEnabled,
                        config.preLowThreshold,
                        config.preHighThreshold
                    )
                    Natives.writealarmsuspension(5, config.urgentLowSnoozeMinutes.toShort())
                    Natives.writealarmsuspension(6, config.veryHighSnoozeMinutes.toShort())
                    Natives.writealarmsuspension(7, config.preLowSnoozeMinutes.toShort())
                    Natives.writealarmsuspension(8, config.preHighSnoozeMinutes.toShort())
                }
            } catch (_: Throwable) {}
        }
    }

    // --- ALARM BEHAVIOR (SOUND / VIBRATION / DURATION) ACTIONS ---

    private fun readAlarmBehavior(): List<AlarmBehavior> {
        return behaviorKinds.map { kind ->
            AlarmBehavior(
                kind = kind,
                sound = try { Natives.alarmhassound(kind) } catch (_: Throwable) { true },
                vibration = try { Natives.alarmhasvibration(kind) } catch (_: Throwable) { true },
                durationSecs = try { Natives.readalarmduration(kind).takeIf { it > 0 } ?: 60 } catch (_: Throwable) { 60 }
            )
        }
    }

    fun behaviorFor(kind: Int): AlarmBehavior? = _alarmBehavior.value.find { it.kind == kind }

    fun updateAlarmBehavior(behavior: AlarmBehavior) {
        _alarmBehavior.value = _alarmBehavior.value.map { if (it.kind == behavior.kind) behavior else it }
        scope.launch(Dispatchers.IO) {
            try {
                if (Applic.Nativesloaded) {
                    // Preserve ringtone URI and flash flag, only sound/vibration change here.
                    val uri = try { Natives.readring(behavior.kind) } catch (_: Throwable) { null } ?: ""
                    val flash = try { Natives.alarmhasflash(behavior.kind) } catch (_: Throwable) { false }
                    Natives.writering(behavior.kind, uri, behavior.sound, flash, behavior.vibration)
                    Natives.writealarmduration(behavior.kind, behavior.durationSecs)
                }
            } catch (_: Throwable) {}
        }
    }

    // --- VOICE OUTPUT ACTIONS ---

    fun setVoiceAnnounce(enabled: Boolean) {
        _voiceAnnounce.value = enabled
        scope.launch(Dispatchers.IO) {
            try {
                if (Applic.Nativesloaded) {
                    // Preserve current rate/pitch, only the on/off state changes here.
                    val speed = try { Natives.getVoiceSpeed().let { if (it > 0) it else 1f } } catch (_: Throwable) { 1f }
                    val pitch = try { Natives.getVoicePitch().let { if (it > 0) it else 1f } } catch (_: Throwable) { 1f }
                    Natives.saveVoice(speed, pitch, 50, 0, enabled)
                }
            } catch (_: Throwable) {}
        }
    }

    fun setSpeakAlarms(enabled: Boolean) {
        _speakAlarms.value = enabled
        scope.launch(Dispatchers.IO) {
            try {
                if (Applic.Nativesloaded) {
                    Natives.setspeakalarms(enabled)
                }
            } catch (_: Throwable) {}
        }
    }

    fun setNfcSound(enabled: Boolean) {
        _hardwareConfig.value = _hardwareConfig.value.copy(nfcSound = enabled)
        scope.launch(Dispatchers.IO) {
            try {
                if (Applic.Nativesloaded) {
                    Natives.setnfcsound(enabled)
                    Applic.RunOnUiThread {
                        Applic.getActivity()?.setnfc()
                    }
                }
            } catch (_: Throwable) {}
        }
    }

    fun setNfcLaunchEnabled(enabled: Boolean) {
        _hardwareConfig.value = _hardwareConfig.value.copy(globalScanStartsApp = enabled)
        scope.launch(Dispatchers.IO) {
            try {
                val component = ComponentName(Applic.app, NFC_LAUNCH_COMPONENT)
                val state = if (enabled) {
                    PackageManager.COMPONENT_ENABLED_STATE_ENABLED
                } else {
                    PackageManager.COMPONENT_ENABLED_STATE_DISABLED
                }
                Applic.app.packageManager.setComponentEnabledSetting(
                    component,
                    state,
                    PackageManager.DONT_KILL_APP
                )
            } catch (_: Throwable) {}
        }
    }

    // --- Glucose meters

    /**
     * Switches the whole meter feature on or off. Switching it off drops the Bluetooth
     * connections but keeps the stored meters, so switching it back on picks them up again.
     */
    fun setMetersEnabled(enabled: Boolean) {
        _metersEnabled.value = enabled
        scope.launch(Dispatchers.IO) {
            MeterSettings.setEnabled(Applic.app, enabled)
            if (enabled) {
                BluetoothGlucoseMeter.startDevices()
            } else {
                MeterBleSupport.stopDiscovery()
                _meterScan.value = MeterScanState()
                _meterRuntimes.value = emptyList()
                BluetoothGlucoseMeter.stopDevices()
            }
        }
        if (enabled) refreshMeters() else _meters.value = emptyList()
    }

    /**
     * Rereads the stored meters and what their connections are doing. The meter list is small, but
     * it comes out of the native settings file, so it is read off the main thread.
     */
    fun refreshMeters() {
        scope.launch(Dispatchers.Default) {
            if (_metersEnabled.value) {
                readMetersNow()
            } else {
                _meters.value = emptyList()
                _meterRuntimes.value = emptyList()
            }
        }
    }

    /** Reads the stored meters and publishes them. Returns the same list it published. */
    private fun readMetersNow(): List<MeterInfo> {
        val list = try {
            val count = Natives.GlucoseMeterCount()
            (0 until count).mapNotNull { index ->
                val name = Natives.GlucoseMeterDeviceName(index) ?: return@mapNotNull null
                MeterInfo(
                    index = index,
                    name = name,
                    address = Natives.GlucoseMeterDeviceAddress(index).orEmpty(),
                    active = Natives.GlucoseMeterGetActive(index),
                    lastReadingTime = Natives.GlucoseMeterGetLastTime(index)
                )
            }
        } catch (_: Throwable) {
            emptyList()
        }
        _meters.value = list
        _meterRuntimes.value = try {
            MeterBleSupport.runtimeStates()
        } catch (_: Throwable) {
            emptyList()
        }
        return list
    }

    /**
     * Turns a meter on or off. A meter that is switched on is talked to right away instead of
     * waiting for the next app start.
     */
    fun setMeterActive(meter: MeterInfo, active: Boolean) {
        try {
            if (Natives.GlucoseMeterSetActive(meter.index, active)) {
                if (active) MeterBleSupport.activate(meter.index, null) else MeterBleSupport.deactivate(meter.index)
            }
        } catch (_: Throwable) {}
        refreshMeters()
    }

    /**
     * Puts a meter that was just scanned into the list and returns it, or null when it could not
     * be added. Adding is what the native call does; the meter stays in the list even when the
     * setup is left unfinished, so the scan is not lost.
     */
    fun addMeter(name: String, address: String?): MeterInfo? {
        val index = try {
            Natives.GlucoseMeterGetIndex(name, address)
        } catch (_: Throwable) {
            -1
        }
        if (index < 0) return null
        val list = readMetersNow()
        refreshMeters()
        return list.firstOrNull { it.index == index }
    }

    /** Forgets a meter, keeping the readings it brought in. */
    fun removeMeter(index: Int) {
        try {
            if (Natives.GlucoseMeterRemoveIndex(index)) MeterBleSupport.restart()
        } catch (_: Throwable) {}
        refreshMeters()
    }

    /**
     * Saves a meter's setup. Setting the cutoff also switches the meter on, because a meter
     * nobody talks to would collect nothing; [found] is the scanned device when the meter was just
     * added, so it does not have to be looked up again to start connecting.
     */
    fun saveMeter(meter: MeterInfo, bloodAfter: Long, found: MeterFound? = null) {
        try {
            Natives.GlucoseMeterSetLastTime(meter.index, bloodAfter)
            MeterBleSupport.activate(meter.index, found)
        } catch (_: Throwable) {}
        refreshMeters()
    }

    /** Searches for meters that can be added. */
    fun startMeterScan() {
        if (!MeterBleSupport.supported) {
            _meterScan.value = MeterScanState(MeterScanStatus.UNSUPPORTED)
            return
        }
        if (!MeterBleSupport.bluetoothEnabled) {
            _meterScan.value = MeterScanState(MeterScanStatus.UNAVAILABLE)
            return
        }
        _meterScan.value = MeterScanState(MeterScanStatus.SCANNING)
        try {
            MeterBleSupport.startDiscovery(false) { found ->
                _meterScan.value = _meterScan.value.copy(
                    found = (_meterScan.value.found.filterNot { it.name == found.name } + found)
                )
            }
        } catch (_: Throwable) {
            _meterScan.value = MeterScanState(MeterScanStatus.UNAVAILABLE)
        }
    }

    fun stopMeterScan() {
        try {
            MeterBleSupport.stopDiscovery()
        } catch (_: Throwable) {}
        _meterScan.value = MeterScanState()
    }

    fun setHour24(use24Hour: Boolean) {
        _displayConfig.value = _displayConfig.value.copy(use24Hour = use24Hour)
        scope.launch(Dispatchers.IO) {
            try {
                if (Applic.Nativesloaded) {
                    Applic.sethour24(use24Hour)
                }
            } catch (_: Throwable) {}
        }
    }

    // --- EXCHANGES ACTIONS ---

    private fun discoverBroadcastReceiverApps(
        action: String,
        selectedPackages: List<String>
    ): List<BroadcastReceiverApp> = try {
        val packageManager = Applic.app.packageManager
        val installedPackages = packageManager
            .queryBroadcastReceivers(Intent(action), 0)
            .asSequence()
            .mapNotNull { it.activityInfo?.packageName }
            .filter { it.isNotBlank() }
            .distinct()
            .toList()
        val installedApps = installedPackages.map { packageName ->
            val label = try {
                val applicationInfo = packageManager.getApplicationInfo(packageName, 0)
                packageManager.getApplicationLabel(applicationInfo).toString().trim()
            } catch (_: Throwable) {
                packageName
            }
            BroadcastReceiverApp(
                packageName = packageName,
                label = label.ifEmpty { packageName }
            )
        }
        val installedPackageSet = installedPackages.toSet()
        val unavailableApps = selectedPackages
            .filterNot { it in installedPackageSet }
            .map { BroadcastReceiverApp(packageName = it, label = it, installed = false) }
        (installedApps + unavailableApps)
            .distinctBy { it.packageName }
            .sortedWith(compareBy({ !it.installed }, { it.label.lowercase() }, { it.packageName }))
    } catch (_: Throwable) {
        emptyList()
    }

    private fun normalizeReceiverPackages(packageNames: Sequence<String>): List<String> = packageNames
        .map { it.trim() }
        .filter { it.isNotEmpty() && it.toByteArray(Charsets.UTF_8).size < 100 }
        .distinct()
        .take(10)
        .toList()

    fun refreshXdripReceiverApps() {
        _xdripReceiverAppsLoading.value = true
        scope.launch(Dispatchers.IO) {
            _xdripReceiverApps.value = discoverBroadcastReceiverApps(
                action = SendLikexDrip.ACTION,
                selectedPackages = _exchanges.value.xdripReceiverPackages
            )
            _xdripReceiverAppsLoading.value = false
        }
    }

    fun setXdripReceivers(packageNames: List<String>) {
        scope.launch(Dispatchers.IO) {
            try {
                if (Applic.Nativesloaded) {
                    val normalizedPackages = normalizeReceiverPackages(packageNames.asSequence())
                    Natives.setxdripRecepters(normalizedPackages.toTypedArray())
                    SendLikexDrip.setreceivers()
                    val savedPackages = normalizeReceiverPackages(Natives.xdripRecepters().asSequence())
                    _exchanges.value = _exchanges.value.copy(
                        xdripBroadcast = savedPackages.isNotEmpty(),
                        xdripReceiverPackages = savedPackages
                    )
                }
            } catch (_: Throwable) {}
        }
    }

    fun refreshGlucodataReceiverApps() {
        _glucodataReceiverAppsLoading.value = true
        scope.launch(Dispatchers.IO) {
            _glucodataReceiverApps.value = discoverBroadcastReceiverApps(
                action = JugglucoSend.ACTION,
                selectedPackages = _exchanges.value.glucodataReceiverPackages
            )
            _glucodataReceiverAppsLoading.value = false
        }
    }

    fun setGlucodataReceivers(packageNames: List<String>) {
        scope.launch(Dispatchers.IO) {
            try {
                if (Applic.Nativesloaded) {
                    val normalizedPackages = normalizeReceiverPackages(packageNames.asSequence())
                    Natives.setglucodataRecepters(normalizedPackages.toTypedArray())
                    JugglucoSend.setreceivers()
                    val savedPackages = normalizeReceiverPackages(Natives.glucodataRecepters().asSequence())
                    _exchanges.value = _exchanges.value.copy(
                        glucodataBroadcast = savedPackages.isNotEmpty(),
                        glucodataReceiverPackages = savedPackages
                    )
                }
            } catch (_: Throwable) {}
        }
    }

    fun setHealthConnect(enabled: Boolean, context: Activity?) {
        _exchanges.value = _exchanges.value.copy(healthConnect = enabled)
        scope.launch(Dispatchers.IO) {
            try {
                if (Applic.Nativesloaded) {
                    SensorBridge.initHealthConnect(context as? MainActivity, enabled)
                }
            } catch (_: Throwable) {}
        }
    }

    fun setLibreViewEnabled(enabled: Boolean) {
        _exchanges.value = _exchanges.value.copy(libreViewEnabled = enabled)
        scope.launch(Dispatchers.IO) {
            try {
                if (Applic.Nativesloaded) {
                    Natives.setuselibreview(enabled)
                }
            } catch (_: Throwable) {}
        }
    }

    /**
     * Writes the LibreView account. Credentials are length checked here, before anything reaches
     * JNI: `libreemail` and `librepass` are fixed size fields in the settings struct that native
     * fills without a bounds check. The email is required to look like an account and the
     * password to be non-trivial as soon as [validateCredentials] is set, which is what the
     * user turning LibreView uploads on means.
     */
    fun saveLibreViewAccount(
        email: String,
        password: String,
        region: LibreRegion,
        accountId: Long,
        manualAccountId: Boolean,
        sendAmounts: Boolean,
        validateCredentials: Boolean
    ): LibreSaveError {
        val address = email.trim()
        // The upper bounds always apply: a longer credential would be copied past a fixed size
        // field in the settings struct. The lower bounds only matter once the user has switched
        // LibreView uploads on, which is what [validateCredentials] says.
        if (address.length > LIBRE_EMAIL_MAX_LENGTH) return LibreSaveError.EMAIL_TOO_LONG
        if (password.length > LIBRE_PASSWORD_MAX_LENGTH) return LibreSaveError.PASSWORD_TOO_LONG
        if (validateCredentials) {
            if (address.length < LIBRE_EMAIL_MIN_LENGTH) return LibreSaveError.EMAIL_TOO_SHORT
            if (password.length < LIBRE_PASSWORD_MIN_LENGTH) return LibreSaveError.PASSWORD_TOO_SHORT
        }
        if (manualAccountId && accountId <= 0L) return LibreSaveError.ACCOUNT_ID_MISSING
        scope.launch(Dispatchers.IO) {
            try {
                if (!Applic.Nativesloaded) return@launch
                Natives.setlibreemail(address)
                Natives.setlibrepass(password)
                Natives.setLibreCountry(region.nativeIndex)
                Natives.setlibreAccountIDnumber(if (manualAccountId) accountId else -1L)
                Natives.setSendNumbers(sendAmounts)
                // Both fields empty means the account was removed: forget what was uploaded
                // for it, exactly like the legacy dialog did.
                if (address.isEmpty() && password.isEmpty()) {
                    Natives.clearlibreFromMSec(0L)
                }
                readLibreViewState()
            } catch (_: Throwable) {}
        }
        return LibreSaveError.NONE
    }

    /** Asks LibreView for the account id belonging to the saved credentials. */
    fun requestLibreViewAccountId() {
        scope.launch(Dispatchers.IO) {
            try {
                if (Applic.Nativesloaded) {
                    Natives.setlibreAccountIDnumber(-1L)
                    Natives.askServerforAccountID()
                }
            } catch (_: Throwable) {}
            readLibreViewState()
        }
    }

    fun setLibreViewRegion(region: LibreRegion) {
        _libreView.value = _libreView.value.copy(region = region)
        scope.launch(Dispatchers.IO) {
            try {
                if (Applic.Nativesloaded) {
                    Natives.setLibreCountry(region.nativeIndex)
                }
            } catch (_: Throwable) {}
        }
    }

    fun setLibreViewUploadCurrent(enabled: Boolean) {
        _libreView.value = _libreView.value.copy(uploadCurrent = enabled)
        scope.launch(Dispatchers.IO) {
            try {
                if (Applic.Nativesloaded) {
                    Natives.setLibreCurrent(enabled)
                }
            } catch (_: Throwable) {}
        }
    }

    fun setLibreViewUploadViewed(enabled: Boolean) {
        _libreView.value = _libreView.value.copy(uploadViewed = enabled)
        scope.launch(Dispatchers.IO) {
            try {
                if (Applic.Nativesloaded) {
                    Natives.setLibreIsViewed(enabled)
                }
            } catch (_: Throwable) {}
        }
    }

    fun setLibreViewSendAmounts(enabled: Boolean) {
        _libreView.value = _libreView.value.copy(sendAmounts = enabled)
        scope.launch(Dispatchers.IO) {
            try {
                if (Applic.Nativesloaded) {
                    Natives.setSendNumbers(enabled)
                }
            } catch (_: Throwable) {}
            readLibreViewState()
        }
    }

    /** Maps one logbook label onto a LibreView record kind, with the carbs weight it is stored with. */
    fun setLibreLabelMapping(index: Int, kind: LibreTreatmentKind, weight: Float) {
        _libreTreatments.value = _libreTreatments.value.map { mapping ->
            if (mapping.index == index) mapping.copy(kind = kind, weight = weight) else mapping
        }
        scope.launch(Dispatchers.IO) {
            try {
                if (Applic.Nativesloaded) {
                    Natives.setlibrenum(LIBRE_NIGHT, index, kind.nativeValue, weight)
                }
            } catch (_: Throwable) {}
            // Native used to assume Aspart for a label mapped as rapid acting insulin, but only as
            // a one-off upgrade step while the calculation was already on, so a mapping made later
            // in Compose was never picked up. The same assumption belongs here, where the mapping
            // is actually changed.
            if (kind == LibreTreatmentKind.RAPID_INSULIN) {
                NativeInsulinOnboard.ensureInsulinType(index, InsulinType.ASPART)
            }
            readLibreViewState()
        }
    }

    /** Starts a LibreView upload in the background, like the uploader does after a scan. */
    fun startLibreViewUpload() {
        scope.launch(Dispatchers.IO) {
            try {
                if (Applic.Nativesloaded) {
                    Natives.wakelibreview(0)
                }
            } catch (_: Throwable) {}
        }
    }

    /**
     * Resends everything from [fromMsec] onwards: everything older than that is forgotten, so the
     * next upload starts over from the chosen moment.
     */
    fun resendLibreViewFrom(fromMsec: Long) {
        scope.launch(Dispatchers.IO) {
            try {
                if (Applic.Nativesloaded) {
                    Natives.clearlibreFromMSec(fromMsec.coerceAtLeast(0L))
                }
            } catch (_: Throwable) {}
        }
    }

    fun setXdripWebServer(enabled: Boolean) {
        _exchanges.value = _exchanges.value.copy(xdripWebServer = enabled)
        scope.launch(Dispatchers.IO) {
            try {
                if (Applic.Nativesloaded) {
                    Natives.setusexdripwebserver(enabled)
                }
            } catch (_: Throwable) {}
        }
    }

    fun restartXdripWebServer() {
        scope.launch(Dispatchers.IO) {
            try {
                if (Applic.Nativesloaded) {
                    Natives.setusexdripwebserver(false)
                    Natives.setusexdripwebserver(true)
                }
            } catch (_: Throwable) {}
        }
    }

    fun uploaderSecret(): String {
        return try { Natives.getnightuploadsecret().orEmpty() } catch (_: Throwable) { "" }
    }

    fun saveUploaderConfig(url: String, secret: String, active: Boolean, v3: Boolean) {
        _uploader.value = _uploader.value.copy(url = url, active = active, v3 = v3)
        scope.launch(Dispatchers.IO) {
            try {
                if (Applic.Nativesloaded) {
                    Natives.setNightUploader(url, secret, active, v3)
                }
            } catch (_: Throwable) {}
        }
    }

    fun setUploaderActive(enabled: Boolean) {
        val current = _uploader.value
        saveUploaderConfig(current.url, uploaderSecret(), enabled, current.v3)
    }

    fun setUploaderPostTreatments(enabled: Boolean) {
        _uploader.value = _uploader.value.copy(postTreatments = enabled)
        scope.launch(Dispatchers.IO) {
            try {
                if (Applic.Nativesloaded) {
                    Natives.setpostTreatments(enabled)
                }
            } catch (_: Throwable) {}
        }
    }

    fun sendUploaderNow() {
        scope.launch(Dispatchers.IO) {
            try {
                if (Applic.Nativesloaded) {
                    Natives.wakeuploader()
                }
            } catch (_: Throwable) {}
        }
    }

    fun resendUploaderData() {
        scope.launch(Dispatchers.IO) {
            try {
                if (Applic.Nativesloaded) {
                    Natives.resetuploader()
                }
            } catch (_: Throwable) {}
        }
    }

    suspend fun testUploader(url: String, secret: String): String = withContext(Dispatchers.IO) {
        val result = try {
            if (Applic.Nativesloaded) NightPost.testconnection(url, secret) else "test failure: not initialized"
        } catch (e: Throwable) {
            e.message.orEmpty()
        }
        _uploaderStatus.value = UploaderStatus(text = result, timeMillis = System.currentTimeMillis())
        result
    }

    fun refreshUploaderStatus() {
        val text = try { NightPost.getstatus() } catch (_: Throwable) { "" }
        val time = try { NightPost.getuploadtime() } catch (_: Throwable) { 0L }
        _uploaderStatus.value = UploaderStatus(text = text.orEmpty(), timeMillis = time)
    }

    fun uploaderTreatmentMappings(): List<TreatmentMapping> {
        val labels = try { Natives.getLabels() } catch (_: Throwable) { null } ?: return emptyList()
        if (labels.size < 2) return emptyList()
        return (0 until labels.size - 1).map { index ->
            TreatmentMapping(
                index = index,
                label = LabelNames.fullName(index, labels[index].orEmpty()),
                kind = try { Natives.getlibrenumkind(1, index) } catch (_: Throwable) { 0 },
                weight = try { Natives.getlibrefoodweight(1, index) } catch (_: Throwable) { 1f }
            )
        }
    }

    fun setUploaderTreatmentMapping(index: Int, kind: Int, weight: Float) {
        scope.launch(Dispatchers.IO) {
            try {
                if (Applic.Nativesloaded) {
                    Natives.setlibrenum(1, index, kind, weight)
                }
            } catch (_: Throwable) {}
            val canSend = try { Natives.canSendNumbers(1) } catch (_: Throwable) { false }
            _uploader.value = _uploader.value.copy(canSendTreatments = canSend)
        }
    }

    fun setLibrelinkBroadcast(enabled: Boolean) {
        _exchanges.value = _exchanges.value.copy(librelinkBroadcast = enabled)
        scope.launch(Dispatchers.IO) {
            try {
                if (Applic.Nativesloaded) {
                    if (enabled) {
                        val intent = Intent(XInfuus.glucoseaction)
                        val receivers = Applic.app.packageManager.queryBroadcastReceivers(intent, 0)
                        val names = receivers.mapNotNull { it.activityInfo?.packageName }.distinct()
                        val targetNames = if (names.isNotEmpty()) names.toTypedArray() else arrayOf("tk.glucodata.dev", "com.freestylelibre.app")
                        Natives.setlibrelinkRecepters(targetNames)
                    } else {
                        Natives.setlibrelinkRecepters(emptyArray())
                    }
                    XInfuus.setlibrenames()
                }
            } catch (_: Throwable) {}
        }
    }

    // --- MIRROR & DATA RELAY ACTIONS ---

    fun refreshMirrorConnections() {
        scope.launch(Dispatchers.IO) {
            val list = mutableListOf<MirrorConnection>()
            try {
                if (Applic.Nativesloaded) {
                    Applic.ensureNetStarted()
                    val count = try { Natives.backuphostNr() } catch (_: Throwable) { 0 }
                    for (i in 0 until count) {
                        val rawIps = try { Natives.getbackupIPs(i) } catch (_: Throwable) { null }
                        val ips = rawIps?.filterNotNull()?.filter { it.isNotBlank() } ?: emptyList()
                        val label = try { Natives.getbackuplabel(i) ?: "" } catch (_: Throwable) { "" }
                        val port = try { Natives.getbackuphostport(i) ?: "" } catch (_: Throwable) { "" }
                        val isReceiver = try { (Natives.getbackuphostreceive(i) and 2) != 0 } catch (_: Throwable) { false }
                        val sendAmounts = try { Natives.getbackuphostnums(i) } catch (_: Throwable) { false }
                        val sendStream = try { Natives.getbackuphoststream(i) } catch (_: Throwable) { false }
                        val sendScans = try { Natives.getbackuphostscans(i) } catch (_: Throwable) { false }
                        val isActive = try { Natives.getbackuphostactive(i) } catch (_: Throwable) { false }
                        val isPassive = try { Natives.getbackuphostpassive(i) } catch (_: Throwable) { false }
                        val isDeactivated = try { Natives.getHostDeactivated(i) } catch (_: Throwable) { false }
                        val status = try { Natives.mirrorStatus(i) ?: "" } catch (_: Throwable) { "" }
                        val iceLabel = try { Natives.getICElabel(i) } catch (_: Throwable) { null }
                        list.add(
                            MirrorConnection(
                                index = i,
                                label = label,
                                ips = ips,
                                port = port,
                                isReceiver = isReceiver,
                                sendAmounts = sendAmounts,
                                sendStream = sendStream,
                                sendScans = sendScans,
                                isActive = isActive,
                                isPassive = isPassive,
                                isDeactivated = isDeactivated,
                                status = status,
                                transport = try {
                                    Natives.getbackuptransport(i)
                                } catch (_: Throwable) {
                                    BleMirror.TRANSPORT_AUTOMATIC
                                },
                                isIce = !iceLabel.isNullOrEmpty()
                            )
                        )
                    }
                }
            } catch (_: Throwable) {}
            _mirrorConnections.value = list
        }
    }

    fun addLocalReceiverConnection(targetPort: String = "17580", label: String = res(R.string.loc_local_production_app)): Boolean {
        return try {
            if (!Applic.Nativesloaded) return false
            val portStr = targetPort.trim().ifEmpty { "17580" }
            val pos = Natives.changebackuphost(
                -1,
                arrayOf("127.0.0.1"),
                1,
                false,
                portStr,
                false,
                false,
                false,
                false,
                true,
                true,
                false,
                null,
                0L,
                label.trim().ifEmpty { res(R.string.loc_local_production_app) },
                false,
                false,
                null,
                false,
                BleMirror.TRANSPORT_TCP,
                false
            )
            if (pos >= 0) {
                BleMirror.configurationChanged(pos, true)
                MessageSender.reinit()
                Applic.switchSync()
                refreshMirrorConnections()
                true
            } else {
                false
            }
        } catch (_: Throwable) {
            false
        }
    }

    fun addLocalSenderConnection(targetPort: String = "17581", label: String = res(R.string.loc_local_dev_build)): Boolean {
        return try {
            if (!Applic.Nativesloaded) return false
            val portStr = targetPort.trim().ifEmpty { "17581" }
            val pos = Natives.changebackuphost(
                -1,
                arrayOf("127.0.0.1"),
                1,
                false,
                portStr,
                true,
                true,
                true,
                false,
                false,
                false,
                false,
                null,
                0L,
                label.trim().ifEmpty { res(R.string.loc_local_dev_build) },
                false,
                false,
                null,
                false,
                BleMirror.TRANSPORT_TCP,
                false
            )
            if (pos >= 0) {
                BleMirror.configurationChanged(pos, true)
                MessageSender.reinit()
                Applic.switchSync()
                refreshMirrorConnections()
                true
            } else {
                false
            }
        } catch (_: Throwable) {
            false
        }
    }

    fun addCustomMirrorConnection(
        ip: String,
        port: String,
        isReceiver: Boolean,
        label: String,
        sendStream: Boolean = true,
        sendScans: Boolean = true,
        sendAmounts: Boolean = true
    ): Boolean {
        return try {
            if (!Applic.Nativesloaded) return false
            val cleanIp = ip.trim().ifEmpty { "127.0.0.1" }
            val cleanPort = port.trim().ifEmpty { "17580" }
            val pos = Natives.changebackuphost(
                -1,
                arrayOf(cleanIp),
                1,
                false,
                cleanPort,
                if (isReceiver) false else sendAmounts,
                if (isReceiver) false else sendStream,
                if (isReceiver) false else sendScans,
                false,
                isReceiver,
                isReceiver,
                false,
                null,
                0L,
                label.trim().ifEmpty { res(if (isReceiver) R.string.loc_mirror_receiver else R.string.loc_mirror_sender) },
                false,
                false,
                null,
                false,
                BleMirror.TRANSPORT_TCP,
                false
            )
            if (pos >= 0) {
                BleMirror.configurationChanged(pos, true)
                MessageSender.reinit()
                Applic.switchSync()
                refreshMirrorConnections()
                true
            } else {
                false
            }
        } catch (_: Throwable) {
            false
        }
    }

    suspend fun mirrorHostEditState(index: Int): MirrorHostEditState? = withContext(Dispatchers.IO) {
        try {
            if (!Applic.Nativesloaded || index < 0) return@withContext null
            val raw = Natives.getMirrorHostEditState(index) ?: return@withContext null
            if (raw.size < Natives.MIRRORSTATE_SIZE) return@withContext null
            fun boolAt(pos: Int) = raw[pos] as? Boolean ?: false
            fun intAt(pos: Int) = (raw[pos] as? Int) ?: 0
            MirrorHostEditState(
                index = index,
                label = raw[Natives.MIRRORSTATE_LABEL] as? String ?: "",
                hasLabel = boolAt(Natives.MIRRORSTATE_HASLABEL),
                ips = (raw[Natives.MIRRORSTATE_IPS] as? Array<*>)
                    ?.mapNotNull { it as? String }
                    ?: emptyList(),
                port = raw[Natives.MIRRORSTATE_PORT] as? String ?: "",
                receiveFrom = intAt(Natives.MIRRORSTATE_RECEIVEFROM),
                activeReceive = intAt(Natives.MIRRORSTATE_ACTIVERECEIVE),
                sendAmounts = boolAt(Natives.MIRRORSTATE_SENDNUMS),
                sendStream = boolAt(Natives.MIRRORSTATE_SENDSTREAM),
                sendScans = boolAt(Natives.MIRRORSTATE_SENDSCANS),
                sendPassive = boolAt(Natives.MIRRORSTATE_SENDPASSIVE),
                restore = boolAt(Natives.MIRRORSTATE_RESTORE),
                startTime = (raw[Natives.MIRRORSTATE_STARTTIME] as? Long) ?: 0L,
                detect = boolAt(Natives.MIRRORSTATE_DETECT),
                testIp = boolAt(Natives.MIRRORSTATE_TESTIP),
                hasHostname = boolAt(Natives.MIRRORSTATE_HOSTNAME),
                iceLabel = raw[Natives.MIRRORSTATE_ICE] as? String ?: "",
                side = boolAt(Natives.MIRRORSTATE_SIDE),
                transport = intAt(Natives.MIRRORSTATE_TRANSPORT),
                bleClient = boolAt(Natives.MIRRORSTATE_BLECLIENT),
                bleReverse = boolAt(Natives.MIRRORSTATE_BLEREVERSE),
                bleUnproven = boolAt(Natives.MIRRORSTATE_BLEUNPROVEN),
                wearOs = boolAt(Natives.MIRRORSTATE_WEAROS),
                deactivated = boolAt(Natives.MIRRORSTATE_DEACTIVATED),
                hasPassword = boolAt(Natives.MIRRORSTATE_HASPASS)
            )
        } catch (_: Throwable) {
            null
        }
    }

    suspend fun saveMirrorConnectionDraft(
        index: Int,
        draft: MirrorConnectionDraft
    ): MirrorSaveResult = withContext(Dispatchers.IO) {
        if (!Applic.Nativesloaded) {
            return@withContext MirrorSaveResult(error = MirrorSaveError.NOT_AVAILABLE)
        }
        val stored = if (index >= 0) mirrorHostEditState(index) else null
        if (index >= 0 && stored == null) {
            return@withContext MirrorSaveResult(error = MirrorSaveError.NOT_AVAILABLE)
        }
        validateMirrorDraft(draft)?.let { error ->
            return@withContext MirrorSaveResult(error = error)
        }
        val usesNetwork = draft.isNetworkTransport
        val ice = usesNetwork && draft.ice
        // The ICE path writes the name unconditionally and would read a null string,
        // so a relay connection without a name still gets an empty one.
        val label = draft.label.trim().ifEmpty { if (ice) "" else null }
        val cleanAddresses = if (usesNetwork && !ice) {
            draft.addresses.map { it.trim() }.filter { it.isNotEmpty() }
        } else {
            emptyList()
        }
        val useHostname = usesNetwork && !ice && draft.useHostname
        val detect = usesNetwork && !ice && !useHostname && draft.detectIp && !draft.activeOnly
        val port = if (usesNetwork && !ice) draft.port.trim() else "0"
        val restore = draft.restore && mirrorRestoreSupported()
        val startTime = when {
            !draft.sendsAnything -> 0L
            draft.dataStart == MirrorDataStart.ALL -> 0L
            draft.dataStart == MirrorDataStart.FROM_NOW -> System.currentTimeMillis() / 1000L
            draft.startTime > 0L -> draft.startTime
            else -> safeStartTime()
        }
        // "side" is the permanent identity of a pair: an edit keeps it, a new
        // connection takes it from the draft (or from the ICE side field).
        val side = if (ice) draft.iceSide else (stored?.side ?: draft.side)
        // Ordinary nearby mirrors on a phone keep their direction for the whole pair.
        val ordinaryNearby = !ice && !Applic.isWearable && !(stored?.wearOs ?: false)
        val bleReverse = if (ordinaryNearby) (stored?.bleReverse ?: draft.bleReverse) else draft.bleReverse
        val bleClient = when {
            Applic.isWearable -> true
            stored?.wearOs == true -> false
            ordinaryNearby -> (!side) xor bleReverse
            stored != null -> stored.bleClient
            else -> !draft.sendScans
        }
        val pos = try {
            Natives.changebackuphost(
                index,
                cleanAddresses.toTypedArray(),
                cleanAddresses.size,
                detect,
                port,
                draft.sendAmounts,
                draft.sendStream,
                draft.sendScans,
                restore,
                draft.receiveFrom,
                draft.activeOnly && !ice,
                draft.passiveOnly && !ice,
                if (draft.usePassword) draft.password else null,
                startTime,
                label,
                draft.testIp && !ice,
                useHostname,
                if (ice) draft.iceLabel else null,
                side,
                draft.transport,
                bleClient
            )
        } catch (_: Throwable) {
            -1
        }
        if (pos < 0) return@withContext MirrorSaveResult(error = MirrorSaveError.fromNative(pos))
        if (ordinaryNearby) {
            try {
                Natives.setbackupblereverse(pos, bleReverse)
            } catch (_: Throwable) {}
        }
        BleMirror.configurationChanged(pos, true)
        MessageSender.reinit()
        Applic.switchSync()
        refreshMirrorConnections()
        MirrorSaveResult(
            index = pos,
            error = MirrorSaveError.NONE,
            partialData = !draft.receiveFrom && !(draft.sendAmounts && draft.sendStream && draft.sendScans),
            blocker = try {
                BleMirror.blockingStatusForConnection(pos)
            } catch (_: Throwable) {
                null
            },
            needsBluetoothPermission = draft.transport == BleMirror.TRANSPORT_BLUETOOTH ||
                    (draft.transport == BleMirror.TRANSPORT_AUTOMATIC && !Applic.isWearable && !isWearOsHost(pos))
        )
    }

    /** Turns a stored connection into the editable draft the editor works on. */
    suspend fun mirrorConnectionDraft(index: Int): MirrorConnectionDraft? = withContext(Dispatchers.IO) {
        val state = mirrorHostEditState(index) ?: return@withContext null
        val storedStart = state.startTime
        val dataStart = when {
            storedStart <= 0L -> MirrorDataStart.ALL
            Math.abs(System.currentTimeMillis() / 1000L - storedStart) < MIRROR_NOW_SECONDS -> MirrorDataStart.FROM_NOW
            else -> MirrorDataStart.SCREEN_POSITION
        }
        MirrorConnectionDraft(
            transport = state.transport,
            label = if (state.hasLabel) state.label else "",
            addresses = if (state.isIce) emptyList() else state.ips.filter { it.isNotBlank() },
            port = state.port,
            detectIp = state.detect,
            testIp = state.testIp,
            useHostname = state.hasHostname,
            activeOnly = state.activeReceive > 0,
            passiveOnly = if (state.isReceiver) state.receiveFrom == 2 else state.sendPassive,
            ice = state.isIce,
            iceLabel = state.iceLabel,
            iceSide = state.side,
            receiveFrom = state.isReceiver,
            sendAmounts = state.sendAmounts,
            sendStream = state.sendStream,
            sendScans = state.sendScans,
            restore = state.restore,
            dataStart = dataStart,
            startTime = storedStart,
            usePassword = state.hasPassword,
            // The editor shows the stored password so it can be replaced or removed,
            // which is how the classic editor has always handled it.
            password = if (state.hasPassword) {
                try {
                    Natives.getbackuppassword(index).orEmpty()
                } catch (_: Throwable) {
                    ""
                }
            } else {
                ""
            },
            side = state.side,
            bleReverse = state.bleReverse,
            bleClient = state.bleClient
        )
    }

    /** The connection code of a stored connection, or null when codes are unavailable. */
    suspend fun mirrorConnectionCode(index: Int): String? = withContext(Dispatchers.IO) {
        if (!mirrorCodesAvailable) return@withContext null
        try {
            Natives.getbackJson(index)?.takeIf { it.isNotBlank() }
        } catch (_: Throwable) {
            null
        }
    }

    /**
     * What the nearby Bluetooth link of a connection is doing. The native side shows
     * this next to the native status, and it is the only place a Bluetooth-only
     * connection says anything at all about itself.
     */
    suspend fun mirrorBluetoothStatus(index: Int): String? = withContext(Dispatchers.IO) {
        if (index < 0 || !Applic.Nativesloaded) return@withContext null
        try {
            BleMirror.statusForConnection(index)?.takeIf { it.isNotBlank() }
        } catch (_: Throwable) {
            null
        }
    }

    /** Creates one of the four ready made connections and returns its connection code. */
    suspend fun createMirrorQuickCode(kind: MirrorQuickCode): MirrorSaveResult = withContext(Dispatchers.IO) {
        if (!Applic.Nativesloaded || !mirrorCodesAvailable) {
            return@withContext MirrorSaveResult(error = MirrorSaveError.NOT_AVAILABLE)
        }
        val pos = try {
            when (kind) {
                MirrorQuickCode.LOCAL_SENDER -> Natives.makeHomeSender()
                MirrorQuickCode.LOCAL_RECEIVER -> Natives.makeHomeReceiver()
                MirrorQuickCode.INTERNET_SENDER -> Natives.makeICESender()
                MirrorQuickCode.INTERNET_RECEIVER -> Natives.makeICEReceiver()
            }
        } catch (_: Throwable) {
            -1
        }
        if (pos < 0) return@withContext MirrorSaveResult(error = MirrorSaveError.fromNative(pos))
        BleMirror.configurationChanged(pos, true)
        MessageSender.reinit()
        Applic.switchSync()
        refreshMirrorConnections()
        MirrorSaveResult(
            index = pos,
            error = MirrorSaveError.NONE,
            code = mirrorConnectionCode(pos),
            needsBluetoothPermission = !Applic.isWearable && !isWearOsHost(pos)
        )
    }

    /**
     * Reads a connection code without changing anything, so the caller can show what
     * will be imported and ask before the data of an existing receiver is replaced.
     */
    suspend fun previewMirrorImport(payload: String): MirrorImportPreview? = withContext(Dispatchers.IO) {
        if (!Applic.Nativesloaded) return@withContext null
        val text = payload.trim()
        val marker = text.lastIndexOf(MIRROR_CODE_SUFFIX)
        if (marker < 0) return@withContext null
        val json = try {
            JSONObject(text.substring(0, marker).trim())
        } catch (_: Throwable) {
            return@withContext null
        }
        fun flag(name: String, fallback: Boolean = false) = json.optBoolean(name, fallback)
        fun text(name: String): String? =
            if (json.isNull(name)) null else json.optString(name).takeIf { it.isNotEmpty() }
        val iceLabel = text("ICElabel")
        val side = if (json.has("side")) flag("side") else flag("scans")
        val namesJson = json.optJSONArray("names")
        val names = if (namesJson == null) {
            emptyList()
        } else {
            (0 until namesJson.length()).map { namesJson.optString(it) }
        }
        val transport = json.optInt("transport", BleMirror.TRANSPORT_AUTOMATIC)
            .takeIf { it in BleMirror.TRANSPORT_AUTOMATIC..BleMirror.TRANSPORT_BLUETOOTH }
            ?: BleMirror.TRANSPORT_AUTOMATIC
        val nums = flag("nums")
        val stream = flag("stream")
        val scans = flag("scans")
        val receive = flag("receive")
        val activeOnly = flag("activeonly")
        val passiveOnly = flag("passiveonly")
        val bleClient = if (json.has("bleclient")) {
            flag("bleclient")
        } else {
            if (iceLabel == null) !side else (activeOnly || (!passiveOnly && receive))
        }
        val bleReverse = if (iceLabel != null) {
            false
        } else if (json.has("blereverse")) {
            flag("blereverse")
        } else {
            bleClient != !side
        }
        MirrorImportPreview(
            draft = MirrorConnectionDraft(
                transport = transport,
                label = text("label").orEmpty(),
                // A code never carries the hostname itself: the peer is told to
                // detect the address instead, so an imported connection looks up.
                addresses = names,
                port = if (iceLabel == null) json.optString("port", "17580") else "0",
                detectIp = flag("detect"),
                testIp = flag("testip"),
                useHostname = false,
                activeOnly = activeOnly,
                passiveOnly = passiveOnly,
                ice = iceLabel != null,
                iceLabel = iceLabel.orEmpty(),
                iceSide = side,
                receiveFrom = receive,
                sendAmounts = nums,
                sendStream = stream,
                sendScans = scans,
                restore = false,
                dataStart = MirrorDataStart.ALL,
                usePassword = text("pass") != null,
                password = text("pass").orEmpty(),
                side = side,
                bleReverse = bleReverse,
                bleClient = bleClient
            ),
            present = presentMirrorData(nums = nums, scans = scans, stream = stream)
        )
    }

    /**
     * The data this device already holds, named the way the native side names it:
     * a type is listed when the connection at hand will not send it again, so the
     * data that is present stays as it is. Receiving starts at the beginning, so
     * the user has to agree before that replaces what is there.
     */
    suspend fun mirrorPresentData(): MirrorPresentData = withContext(Dispatchers.IO) {
        if (!Applic.Nativesloaded) return@withContext MirrorPresentData.NONE
        presentMirrorData(nums = false, scans = false, stream = false)
    }

    private fun presentMirrorData(nums: Boolean, scans: Boolean, stream: Boolean) =
        MirrorPresentData(
            amounts = !nums && nativeHasAmounts(),
            scans = !scans && nativeHasScans(),
            stream = !stream && nativeHasStream()
        )

    /** Adds the connection described by a code the user confirmed to import. */
    suspend fun importMirrorCode(preview: MirrorImportPreview): MirrorSaveResult {
        val result = saveMirrorConnectionDraft(-1, preview.draft)
        if (result.ok && !preview.draft.ice) {
            // A code carries the pair direction and the preferred role of the peer.
            // The editor derives both itself, an import takes them over as given.
            try {
                Natives.setbackupblereverse(result.index, preview.draft.bleReverse)
                if (Applic.isWearable) {
                    Natives.setbackupbleclient(result.index, preview.draft.bleClient)
                }
            } catch (_: Throwable) {}
            BleMirror.configurationChanged(result.index, true)
            refreshMirrorConnections()
        }
        return result
    }

    fun setMirrorConnectionDeactivated(index: Int, deactivated: Boolean) {
        scope.launch(Dispatchers.IO) {
            try {
                if (Applic.Nativesloaded && index >= 0) {
                    Natives.setHostDeactivated(index, deactivated)
                    BleMirror.configurationChanged()
                    refreshMirrorConnections()
                }
            } catch (_: Throwable) {}
        }
    }

    /** Whether this build can hand out and read connection codes (not on the watch). */
    fun mirrorCodesSupported(): Boolean = mirrorCodesAvailable

    fun mirrorRestoreSupported(): Boolean = try {
        Applic.Nativesloaded && Natives.backuphasrestore()
    } catch (_: Throwable) {
        false
    }

    private fun validateMirrorDraft(draft: MirrorConnectionDraft): MirrorSaveError? {
        val label = draft.label.trim()
        if (draft.needsLabel && label.isEmpty()) return MirrorSaveError.LABEL_REQUIRED
        if (label.length > MIRROR_LABEL_MAX_LENGTH) return MirrorSaveError.LABEL_TOO_LONG
        if (draft.transport == BleMirror.TRANSPORT_BLUETOOTH &&
            (!draft.usePassword || draft.password.isEmpty())
        ) {
            return MirrorSaveError.PASSWORD_REQUIRED
        }
        if (draft.usePassword && draft.password.length > MIRROR_PASSWORD_MAX_LENGTH) {
            return MirrorSaveError.PASSWORD_TOO_LONG
        }
        if (!draft.receiveFrom && !draft.sendsAnything) return MirrorSaveError.NOTHING_SELECTED
        if (draft.receiveFrom && draft.sendAmounts && draft.sendStream && draft.sendScans) {
            return MirrorSaveError.ALL_DATA_SENT
        }
        if (!draft.isNetworkTransport) return null
        val ice = draft.ice
        if (ice) {
            if (draft.iceLabel.length < MIRROR_ICE_LABEL_MIN_LENGTH) return MirrorSaveError.ICE_LABEL_TOO_SHORT
            if (draft.iceLabel.length > MIRROR_ICE_LABEL_MAX_LENGTH) return MirrorSaveError.ICE_LABEL_TOO_LONG
            return null
        }
        val detect = draft.detectIp && !draft.activeOnly
        val used = draft.addresses.count { it.isNotBlank() }
        val maxAddresses = MIRROR_MAX_ADDRESSES - (if (label.isEmpty()) 0 else 1) - (if (detect) 1 else 0)
        if (used > maxAddresses) return MirrorSaveError.TOO_MANY_ADDRESSES
        if (draft.useHostname) {
            val hostname = draft.addresses.firstOrNull { it.isNotBlank() }.orEmpty()
            if (hostname.isEmpty()) return MirrorSaveError.NO_ADDRESS
            if (hostname.length > MIRROR_HOSTNAME_MAX_LENGTH) return MirrorSaveError.HOSTNAME_TOO_LONG
            return null
        }
        if ((draft.testIp && !detect) || draft.activeOnly) {
            if (used == 0) return MirrorSaveError.NO_ADDRESS
        }
        if (!draft.passiveOnly) {
            val portNumber = draft.port.trim().toIntOrNull()
            if (portNumber == null || portNumber !in 1024..65535) return MirrorSaveError.INVALID_PORT
        }
        return null
    }

    private fun isWearOsHost(index: Int): Boolean = try {
        Natives.isWearOS(index)
    } catch (_: Throwable) {
        false
    }

    private fun safeStartTime(): Long = try {
        Natives.getstarttime() / 1000L
    } catch (_: Throwable) {
        0L
    }

    private fun nativeHasScans(): Boolean = try {
        Natives.hasscans()
    } catch (_: Throwable) {
        false
    }

    private fun nativeHasAmounts(): Boolean = try {
        numio.hasNumdata()
    } catch (_: Throwable) {
        false
    }

    private fun nativeHasStream(): Boolean = try {
        Natives.hasstreamed()
    } catch (_: Throwable) {
        false
    }

    fun deleteMirrorConnection(index: Int) {
        scope.launch(Dispatchers.IO) {
            try {
                if (Applic.Nativesloaded && index >= 0) {
                    Natives.deletebackuphost(index)
                    BleMirror.configurationChanged()
                    MessageSender.reinit()
                    refreshMirrorConnections()
                }
            } catch (_: Throwable) {}
        }
    }

    fun resetMirrorConnection(index: Int) {
        scope.launch(Dispatchers.IO) {
            try {
                if (Applic.Nativesloaded && index >= 0) {
                    Natives.resetbackuphost(index)
                    Applic.switchSync()
                    MessageSender.reinit()
                    refreshMirrorConnections()
                }
            } catch (_: Throwable) {}
        }
    }

    fun setMirrorReceivePort(port: String): Boolean {
        return try {
            val cleanPort = port.trim()
            val num = cleanPort.toIntOrNull()
            if (num != null && num in 1024..65535 &&
                Natives.setreceiveport(cleanPort) == Natives.RECEIVEPORT_OK
            ) {
                MessageSender.reinit()
                true
            } else {
                false
            }
        } catch (_: Throwable) {
            false
        }
    }

    fun openWebServerConfig(activity: Activity) {
        if (activity is MainActivity) {
            try {
                Nightscout.show(activity, activity.window.decorView)
            } catch (_: Throwable) {}
        }
    }

    // --- SMARTWATCH & WEAR OS ACTIONS ---

    fun refreshWearDevices() {
        scope.launch(Dispatchers.IO) {
            try {
                val list = mutableListOf<WearWatchDevice>()
                val targets = LinkedHashMap<String, Pair<String, Boolean>>()

                val nodes = WatchBridge.getWearNodes()
                for (node in nodes) {
                    targets[node.id] = Pair(node.displayName, MessageSender.isGalaxy(node))
                }
                for (label in WatchBridge.getBleWatchLabels()) {
                    targets.putIfAbsent(label, Pair(label, WatchBridge.isGalaxyDefault()))
                }

                val mirrorCount = try { Natives.backuphostNr() } catch (_: Throwable) { 0 }
                val mirrorMap = mutableMapOf<String, Int>()
                for (i in 0 until mirrorCount) {
                    val isWearOS = try { Natives.isWearOS(i) } catch (_: Throwable) { false }
                    val label = try { Natives.getbackuplabel(i) ?: "" } catch (_: Throwable) { "" }
                    if (isWearOS && label.isNotBlank()) {
                        mirrorMap[label] = i
                        targets.putIfAbsent(label, Pair(label, WatchBridge.isGalaxyDefault()))
                    }
                }

                for ((id, pair) in targets) {
                    val displayName = pair.first
                    val isGalaxy = pair.second
                    val dirVal = try { Natives.directsensorwatch(id) } catch (_: Throwable) { 0 }
                    val isDirectSensor = dirVal > 0
                    val numsVal = try { Natives.hasWatchNums(id) } catch (_: Throwable) { 0 }
                    val isEnterNums = numsVal > 0

                    val mirrorIndex = mirrorMap[id] ?: -1
                    var status = ""
                    var ips = emptyList<String>()
                    var isConnected = false
                    if (mirrorIndex >= 0) {
                        status = try { Natives.mirrorStatus(mirrorIndex) ?: "" } catch (_: Throwable) { "" }
                        val rawIps = try { Natives.getbackupIPs(mirrorIndex) } catch (_: Throwable) { null }
                        ips = rawIps?.filterNotNull()?.filter { it.isNotBlank() } ?: emptyList()
                        val isActive = try { Natives.getbackuphostactive(mirrorIndex) } catch (_: Throwable) { false }
                        val isLive = status.contains("live socket: true", ignoreCase = true) ||
                                status.contains("TCP/IP live socket</b>: true", ignoreCase = true) ||
                                status.contains("Direct Bluetooth (BLE GATT)=true", ignoreCase = true) ||
                                status.contains("Messages (Wear OS MessageClient)=true", ignoreCase = true)
                        isConnected = isLive || isActive
                    }

                    list.add(
                        WearWatchDevice(
                            id = id,
                            displayName = if (displayName.isNotBlank() && displayName != id) "$displayName ($id)" else id,
                            isDirectSensor = isDirectSensor,
                            isEnterNumsOnWatch = isEnterNums,
                            isGalaxy = isGalaxy,
                            mirrorIndex = mirrorIndex,
                            mirrorStatus = status,
                            mirrorIps = ips,
                            isConnected = isConnected,
                            transport = if (mirrorIndex >= 0) WatchBridge.getWatchTransport(mirrorIndex) else -1
                        )
                    )
                }
                _wearDevices.value = list
                updateWearDiagnosticInfo(targets.size)
            } catch (th: Throwable) {
                Log.stack("GlucoseRepository", th)
            }
        }
    }

    private fun updateWearDiagnosticInfo(reachableNodesCount: Int) {
        val port = try {
            Natives.getreceiveport() ?: ""
        } catch (_: Throwable) { "" }
        val isReceiverEnabled = WatchBridge.isWearOsEnabled()
        val appId = try { Applic.app.packageName ?: "" } catch (_: Throwable) { "" }
        val version = try {
            val pInfo = Applic.app.packageManager.getPackageInfo(appId, 0)
            pInfo.versionName ?: ""
        } catch (_: Throwable) { "" }

        _wearDiagnosticInfo.value = WearDiagnosticInfo(
            phoneAppId = appId,
            phoneVersion = version,
            mirrorPort = if (port.isNotBlank()) port else if (BuildConfig.DEBUG) "9113" else "8795",
            isReceiverServiceEnabled = isReceiverEnabled,
            reachableWearNodesCount = reachableNodesCount
        )
    }

    fun setWearOsEnabled(context: Context, enabled: Boolean) {
        _watchConfig.value = _watchConfig.value.copy(wearOsEnabled = enabled)
        scope.launch(Dispatchers.IO) {
            WatchBridge.setWearOsEnabled(context, enabled)
            delay(500L)
            refreshWearDevices()
        }
    }

    fun scanForWatches() {
        scope.launch(Dispatchers.IO) {
            WatchBridge.searchWatches()
            delay(1_000L)
            refreshWearDevices()
        }
    }

    fun setWatchDirectSensor(watchId: String, direct: Boolean, isGalaxy: Boolean, hasWatchNums: Boolean) {
        scope.launch(Dispatchers.IO) {
            WatchBridge.setWatchDirectSensor(watchId, direct, isGalaxy, hasWatchNums)
            delay(500L)
            refreshWearDevices()
        }
    }

    fun setWatchEnterNums(watchId: String, watchNums: Boolean, direct: Boolean, isGalaxy: Boolean) {
        scope.launch(Dispatchers.IO) {
            WatchBridge.setWatchEnterNums(watchId, watchNums, direct, isGalaxy)
            delay(500L)
            refreshWearDevices()
        }
    }

    fun setWatchTransport(watchId: String, mirrorIndex: Int, transport: Int) {
        _wearDevices.value = _wearDevices.value.map {
            if (it.id == watchId) it.copy(transport = transport) else it
        }
        scope.launch(Dispatchers.IO) {
            WatchBridge.setWatchTransport(watchId, mirrorIndex, transport)
            delay(500L)
            refreshWearDevices()
        }
    }

    fun initWatchApp(watchId: String, isGalaxy: Boolean) {
        scope.launch(Dispatchers.IO) {
            WatchBridge.initWatchApp(watchId, isGalaxy)
            delay(1_000L)
            refreshWearDevices()
        }
    }

    fun syncWatch(watchId: String) {
        scope.launch(Dispatchers.IO) {
            WatchBridge.syncWatch(watchId)
            delay(1_000L)
            refreshWearDevices()
        }
    }

    fun resetWatchDefaults(watchId: String, isGalaxy: Boolean, context: Context) {
        scope.launch(Dispatchers.IO) {
            WatchBridge.resetWatchDefaults(watchId, isGalaxy, context)
            delay(1_000L)
            refreshWearDevices()
        }
    }

    fun setWatchdripEnabled(enabled: Boolean) {
        _watchConfig.value = _watchConfig.value.copy(watchdripEnabled = enabled)
        scope.launch(Dispatchers.IO) {
            WatchBridge.setWatchdrip(enabled)
        }
    }

    fun setGadgetbridgeEnabled(enabled: Boolean) {
        _watchConfig.value = _watchConfig.value.copy(gadgetbridgeEnabled = enabled)
        scope.launch(Dispatchers.IO) {
            WatchBridge.setGadgetbridge(enabled)
        }
    }

    fun setGarminEnabled(enabled: Boolean) {
        _watchConfig.value = _watchConfig.value.copy(garminEnabled = enabled)
        scope.launch(Dispatchers.IO) {
            GarminBridge.setEnabled(enabled)
        }
    }

    fun setSeparateAlerts(enabled: Boolean) {
        _watchConfig.value = _watchConfig.value.copy(separateAlerts = enabled)
        scope.launch(Dispatchers.IO) {
            WatchBridge.setSeparateAlerts(enabled)
        }
    }

    fun setNotifyWatch(enabled: Boolean) {
        _watchConfig.value = _watchConfig.value.copy(notifyWatch = enabled)
        scope.launch(Dispatchers.IO) {
            WatchBridge.setNotifyWatch(enabled)
        }
    }

    // --- GARMIN ACTIONS ---

    /**
     * Reads the Garmin transport. The status screen calls this about once a second
     * while it is open, because a watch answers, retries and reports state on its
     * own schedule and there is no callback to wait for.
     */
    fun refreshGarminStatus() {
        scope.launch(Dispatchers.IO) {
            _garminStatus.value = GarminBridge.read()
        }
    }

    /**
     * Looks for Garmin Connect devices paired since start-up. This asks the SDK
     * about every known device, so it belongs on entering the screen, not on every
     * refresh tick.
     */
    fun discoverGarminDevices() {
        scope.launch(Dispatchers.IO) {
            GarminBridge.refreshDevices()
            _garminStatus.value = GarminBridge.read()
        }
    }

    fun setGarminActive(peerId: Long, active: Boolean) {
        scope.launch(Dispatchers.IO) {
            GarminBridge.setActive(peerId, active)
            _garminStatus.value = GarminBridge.read()
        }
    }

    fun setGarminGlucose(peerId: Long, enabled: Boolean) {
        scope.launch(Dispatchers.IO) {
            GarminBridge.setGlucose(peerId, enabled)
            _garminStatus.value = GarminBridge.read()
        }
    }

    /**
     * Hands the active Libre 3 sensor to the watch. The phone only lets go once
     * the watch really has the sensor, so the result decides what the screen says.
     */
    suspend fun requestGarminLibre3Direct(peerId: Long, enabled: Boolean): GarminLibre3Result =
        withContext(Dispatchers.IO) {
            val result = GarminBridge.setLibre3Direct(peerId, enabled)
            _garminStatus.value = GarminBridge.read()
            result
        }

    fun setGarminNumbersDevice(peerId: Long, enabled: Boolean) {
        scope.launch(Dispatchers.IO) {
            GarminBridge.setNumbersDevice(peerId, enabled)
            _garminStatus.value = GarminBridge.read()
        }
    }

    fun setGarminTransportMode(mode: Int) {
        _garminStatus.value = _garminStatus.value.copy(transportMode = mode)
        scope.launch(Dispatchers.IO) {
            GarminBridge.setTransportMode(mode)
            _garminStatus.value = GarminBridge.read()
        }
    }

    fun syncGarmin(peerId: Long) {
        scope.launch(Dispatchers.IO) {
            GarminBridge.sync(peerId)
            _garminStatus.value = GarminBridge.read()
        }
    }

    fun sendNextGarminMessage(peerId: Long) {
        scope.launch(Dispatchers.IO) {
            GarminBridge.sendNextMessage(peerId)
            _garminStatus.value = GarminBridge.read()
        }
    }

    fun reinitGarmin(peerId: Long) {
        scope.launch(Dispatchers.IO) {
            GarminBridge.reinit(peerId)
            _garminStatus.value = GarminBridge.read()
        }
    }

    fun setGarminDarkMode(peerId: Long, black: Boolean) {
        _garminStatus.value = _garminStatus.value.copy(
            watches = _garminStatus.value.watches.map {
                if (it.id == peerId) it.copy(darkMode = black) else it
            }
        )
        scope.launch(Dispatchers.IO) {
            GarminBridge.setDarkMode(peerId, black)
            _garminStatus.value = GarminBridge.read()
        }
    }

    /**
     * Stores the ConnectIQ application id and restarts the transport, so a watch
     * that was left talking to another application starts over cleanly. A null id
     * means the default one.
     */
    suspend fun saveGarminAppId(id: String?): Boolean = withContext(Dispatchers.IO) {
        val saved = GarminBridge.saveAppId(id)
        if (saved) {
            GarminBridge.restartTransport()
            _garminStatus.value = GarminBridge.read()
        }
        saved
    }

    fun refreshGarminShortcuts() {
        scope.launch(Dispatchers.IO) {
            _garminShortcuts.value = GarminBridge.shortcuts()
        }
    }

    suspend fun saveGarminShortcuts(shortcuts: List<GarminShortcut>): GarminShortcutError? =
        withContext(Dispatchers.IO) {
            val error = GarminBridge.saveShortcuts(shortcuts)
            _garminShortcuts.value = GarminBridge.shortcuts()
            error
        }

    /** The native help pages only exist in the phone build; elsewhere this is null. */
    fun garminHelpHtml(name: String): String? = try {
        GarminBridge.helpHtml(name)
    } catch (_: Throwable) {
        null
    }

    // --- DISPLAY & UI ACTIONS ---

    fun setFloatingGlucose(enabled: Boolean, activity: Activity) {
        _displayConfig.value = _displayConfig.value.copy(floatingGlucose = enabled)
        scope.launch(Dispatchers.Main) {
            try {
                tk.glucodata.Floating.setfloatglucose(activity, enabled)
            } catch (_: Throwable) {}
        }
    }

    fun setStatusBarNotification(enabled: Boolean) {
        _displayConfig.value = _displayConfig.value.copy(statusBarNotification = enabled)
        scope.launch(Dispatchers.IO) {
            try {
                Notify.glucosestatus(enabled)
            } catch (_: Throwable) {}
        }
    }

    fun setSystemUiFullscreen(enabled: Boolean, activity: Activity?) {
        _displayConfig.value = _displayConfig.value.copy(systemUiFullscreen = enabled)
        scope.launch(Dispatchers.Main) {
            try {
                if (Applic.Nativesloaded) {
                    // setSystemUI(true) reveals the bars; fullscreen hides them.
                    SensorBridge.setSystemUi(activity as? MainActivity, !enabled)
                }
            } catch (_: Throwable) {}
        }
    }

    fun setInvertColors(enabled: Boolean) {
        _displayConfig.value = _displayConfig.value.copy(invertColors = enabled)
        scope.launch(Dispatchers.IO) {
            try {
                if (Applic.Nativesloaded) {
                    Natives.setInvertColors(enabled)
                }
            } catch (_: Throwable) {}
        }
    }

    fun setCalibrationEnabled(enabled: Boolean) {
        _displayConfig.value = _displayConfig.value.copy(calibrationEnabled = enabled)
        scope.launch(Dispatchers.IO) {
            try {
                if (Applic.Nativesloaded) {
                    Natives.setDoCalibrate(enabled)
                    Natives.setshowcalibratedstream(enabled)
                }
            } catch (_: Throwable) {}
        }
    }

    fun setBloodLabelIndex(index: Int) {
        setLabelForType(LogType.BLOOD_GLUCOSE, index)
    }

    // --- LOGBOOK LABELS ---

    /**
     * Saves label [index] (a new one at the end when below zero) as [name], shown as [shortName]
     * where space is tight. A blank short name is derived from the name, and one that does not fit
     * native's 11 bytes is cut to fit, so any name can be saved. [roundTo] is the Garmin rounding
     * step and [weight] the classic graph's height factor, see [tk.glucodata.ui.model.LogLabel].
     */
    fun saveLabel(index: Int, name: String, shortName: String, weight: Float, roundTo: Float): LabelSaveError? {
        val config = currentLabels()
        if (!config.editable) return LabelSaveError.READ_ONLY
        val full = name.trim().take(LabelNames.MAX_NAME_LENGTH)
        if (full.isEmpty()) return LabelSaveError.EMPTY
        val short = LabelNames.fitShort(shortName).ifEmpty { LabelNames.deriveShort(full) }
        if (short.isEmpty()) return LabelSaveError.EMPTY
        val position = if (index in config.labels.indices) index else config.labels.size
        if (position >= LabelConfig.MAX_LABELS) return LabelSaveError.TOO_MANY
        val saved = try {
            Applic.Nativesloaded && Natives.setlabel(position, short, roundTo, weight.coerceAtLeast(0f))
        } catch (_: Throwable) {
            false
        }
        if (!saved) return LabelSaveError.FAILED
        LabelNames.store(position, short, full)
        onLabelsChanged()
        return null
    }

    /** Removes the last label; native keeps labels as a list that only shrinks from the end. */
    fun deleteLastLabel(): Boolean {
        val config = currentLabels()
        val last = config.labels.lastIndex
        if (!config.canDelete(last)) return false
        try {
            Natives.setnrlabel(last)
        } catch (_: Throwable) {
            return false
        }
        LabelNames.forget(last)
        onLabelsChanged()
        return true
    }

    /**
     * Saves new entries of [type] under label [index]. Carbs and finger-pricks are the native meal
     * and blood labels; bolus and basal are kept on this side and handed to a mirroring watch.
     */
    fun setLabelForType(type: LogType, index: Int) {
        val config = currentLabels()
        if (index !in config.labels.indices || config.roleOf(index, except = type) != null) return
        if (!config.editable && type != LogType.BLOOD_GLUCOSE) return
        try {
            when (type) {
                LogType.CARBS, LogType.MEAL -> Natives.setmealvar(index.toByte())
                LogType.BLOOD_GLUCOSE -> Natives.setbloodvar(index.toByte())
                LogType.RAPID_INSULIN, LogType.BASAL_INSULIN -> {
                    NativeLabels.setInsulinLabel(type, index)
                    // The calculation reads the insulin types out of native settings rather than
                    // out of this choice, so a bolus label that carries no insulin would add
                    // nothing to the insulin on board. Aspart is the one native assumes for a
                    // label mapped as rapid acting insulin, and a type the user picked is kept.
                    if (type == LogType.RAPID_INSULIN) {
                        NativeInsulinOnboard.ensureInsulinType(index, InsulinType.ASPART)
                    }
                    // Handed to a mirroring watch by onLabelsChanged below.
                }
                LogType.CUSTOM -> return
            }
        } catch (_: Throwable) {
            return
        }
        _labelConfig.value = config.copy(
            carbsLabel = if (type == LogType.CARBS) index else config.carbsLabel,
            bolusLabel = if (type == LogType.RAPID_INSULIN) index else config.bolusLabel,
            basalLabel = if (type == LogType.BASAL_INSULIN) index else config.basalLabel,
            bloodLabel = if (type == LogType.BLOOD_GLUCOSE) index else config.bloodLabel
        )
        onLabelsChanged()
    }

    /**
     * Hands changed labels to the mirrors and the Garmin watch, the way the classic label screen
     * did on closing, and reloads everything that shows a label.
     */
    private fun onLabelsChanged() {
        scope.launch(Dispatchers.IO) {
            sendLabels()
            reloadLabels()
            _libreTreatments.value = readLibreTreatments()
            loadLogsFromNative()
        }
    }

    /** Hands the labels to mirroring devices: native ones the short names, the watch the full ones too. */
    private fun sendLabels() {
        try {
            if (Natives.shouldsendlabels()) {
                Applic.wakemirrors()
                Applic.app?.sendlabels()
            }
        } catch (_: Throwable) {}
        DisplaySync.onLocalLabelChange()
    }

    // --- INSULIN ONBOARD ---

    /**
     * Turns the insulin on board calculation on or off.
     *
     * Returns whether native accepted it: while no label holds an insulin type there is nothing to
     * calculate with, and native turns the request down rather than storing a flag that would
     * show nothing. The screen that offers the switch says so instead of snapping back silently.
     */
    fun setInsulinOnboard(enabled: Boolean): Boolean {
        val accepted = NativeInsulinOnboard.setEnabled(enabled)
        if (accepted) refreshInsulinOnboard()
        return accepted
    }

    /** The insulin [index] holds for the calculation, or [InsulinType.NONE] where native has none. */
    fun insulinTypeOf(index: Int): InsulinType = NativeInsulinOnboard.insulinType(index)

    /** Stores the insulin label [index] holds and recalculates what is on board. */
    fun setInsulinType(index: Int, type: InsulinType) {
        if (!NativeInsulinOnboard.setInsulinType(index, type)) return
        refreshInsulinOnboard()
    }

    /**
     * Re-reads the flag and the value from native. Safe to call as often as the screen needs it:
     * while the calculation is off it publishes nothing at all.
     */
    fun refreshInsulinOnboard() {
        scope.launch(Dispatchers.IO) { publishInsulinOnboard() }
    }

    /** The publishing half of [refreshInsulinOnboard], for the calls already off the main thread. */
    private fun publishInsulinOnboard() {
        val enabled = NativeInsulinOnboard.isEnabled()
        _insulinOnboardEnabled.value = enabled
        _insulinOnboard.value = if (enabled) {
            NativeInsulinOnboard.valueAt(System.currentTimeMillis())
        } else {
            null
        }
    }

    fun setCalibratePastReadings(enabled: Boolean) {
        _displayConfig.value = _displayConfig.value.copy(calibratePastReadings = enabled)
        scope.launch(Dispatchers.IO) {
            try {
                if (Applic.Nativesloaded) {
                    Natives.setCalibratePast(enabled)
                }
            } catch (_: Throwable) {}
        }
    }

    fun setCalibrateAllValues(enabled: Boolean) {
        _displayConfig.value = _displayConfig.value.copy(calibrateAllValues = enabled)
        scope.launch(Dispatchers.IO) {
            try {
                if (Applic.Nativesloaded) {
                    Natives.setAllValues(enabled)
                }
            } catch (_: Throwable) {}
        }
    }

    // Only true once: before the user has ever been asked, and only while the feature is off.
    fun shouldPromptCalibrationEnable(): Boolean {
        if (_displayConfig.value.calibrationEnabled) return false
        return try {
            !Applic.app.getSharedPreferences(CALIBRATION_PREFS, Context.MODE_PRIVATE)
                .getBoolean(KEY_CALIBRATION_PROMPT_SHOWN, false)
        } catch (_: Throwable) {
            false
        }
    }

    fun markCalibrationPromptShown() {
        try {
            Applic.app.getSharedPreferences(CALIBRATION_PREFS, Context.MODE_PRIVATE)
                .edit()
                .putBoolean(KEY_CALIBRATION_PROMPT_SHOWN, true)
                .apply()
        } catch (_: Throwable) {}
    }

    fun toggleGraphLayer(layer: String, enabled: Boolean) {
        val current = _displayConfig.value
        val updated = when (layer) {
            "scans" -> current.copy(showScans = enabled)
            "calibratedscans" -> current.copy(showCalibratedScans = enabled)
            "stream" -> current.copy(showStream = enabled)
            "calibrated", "calibratedstream" -> current.copy(showCalibratedStream = enabled)
            "history" -> current.copy(showHistory = enabled)
            "calibratedhistory" -> current.copy(showCalibratedHistory = enabled)
            "amounts" -> current.copy(showAmounts = enabled)
            "meals" -> current.copy(showMeals = enabled)
            "alerts" -> current.copy(showAlertLines = enabled)
            else -> current
        }
        _displayConfig.value = updated
        scope.launch(Dispatchers.IO) {
            if (layer == "alerts") {
                try {
                    Applic.app.getSharedPreferences(UI_PREFS, Context.MODE_PRIVATE)
                        .edit()
                        .putBoolean(KEY_SHOW_ALERT_LINES, enabled)
                        .apply()
                } catch (_: Throwable) {}
            }
            try {
                if (Applic.Nativesloaded) {
                    when (layer) {
                        "scans" -> Natives.setshowscans(enabled)
                        "calibratedscans" -> Natives.setshowcalibratedscans(enabled)
                        "stream" -> Natives.setshowstream(enabled)
                        "calibrated", "calibratedstream" -> Natives.setshowcalibratedstream(enabled)
                        "history" -> Natives.setshowhistories(enabled)
                        "calibratedhistory" -> Natives.setshowcalibratedhistories(enabled)
                        "amounts" -> Natives.setshownumbers(enabled)
                        "meals" -> Natives.setshowmeals(enabled)
                    }
                }
            } catch (_: Throwable) {}
        }
    }

    fun setMinimalistUnits(enabled: Boolean) {
        _displayConfig.value = _displayConfig.value.copy(minimalistUnits = enabled)
        scope.launch(Dispatchers.IO) {
            try {
                Applic.app.getSharedPreferences(UI_PREFS, Context.MODE_PRIVATE)
                    .edit()
                    .putBoolean(KEY_MINIMALIST_UNITS, enabled)
                    .apply()
            } catch (_: Throwable) {}
        }
    }

    /**
     * @param fromRemote true when the change arrived from the paired device, so it is not sent
     *   back out again.
     */
    fun setDeltaCalculation(calculation: DeltaCalculation, fromRemote: Boolean = false) {
        _displayConfig.value = _displayConfig.value.copy(deltaCalculation = calculation)
        scope.launch(Dispatchers.IO) {
            try {
                Applic.app.getSharedPreferences(UI_PREFS, Context.MODE_PRIVATE)
                    .edit()
                    .putInt(KEY_DELTA_CALCULATION, calculation.minutes)
                    .apply()
            } catch (_: Throwable) {}
            if (!fromRemote) {
                try {
                    DisplaySync.onLocalDeltaChange(calculation)
                } catch (_: Throwable) {}
            }
        }
    }

    // --- GRAPH NAVIGATION ACTIONS ---

    fun jumpToNow() {
        scope.launch(Dispatchers.IO) {
            try {
                if (Applic.Nativesloaded) {
                    Natives.settonow()
                }
            } catch (_: Throwable) {}
        }
    }

    fun navigateDays(days: Int) {
        scope.launch(Dispatchers.IO) {
            try {
                if (Applic.Nativesloaded) {
                    if (days < 0) Natives.prevday(-days) else Natives.nextday(days)
                }
            } catch (_: Throwable) {}
        }
    }

    fun showLastScan() {
        scope.launch(Dispatchers.IO) {
            try {
                if (Applic.Nativesloaded) {
                    Natives.showlastscan()
                }
            } catch (_: Throwable) {}
        }
    }

    fun moveToDate(year: Int, month: Int, day: Int) {
        scope.launch(Dispatchers.IO) {
            try {
                if (Applic.Nativesloaded) {
                    val start = Natives.getstarttime()
                    Natives.movedate(start, year, month, day)
                }
            } catch (_: Throwable) {}
        }
    }

    fun searchGlucose(
        label: Int = -1,
        under: Float = 0f,
        above: Float = 0f,
        keyword: String = "",
        typeLabels: Map<LogType, String> = emptyMap()
    ): List<Long> {
        val matches = mutableListOf<Long>()

        // 1. Search in glucose readings
        if (under > 0f || above > 0f) {
            for (pt in _readings.value) {
                if (under > 0f && pt.valueMgDl <= under) {
                    matches.add(pt.timestamp)
                } else if (above > 0f && pt.valueMgDl >= above) {
                    matches.add(pt.timestamp)
                }
            }
        }

        // 2. Search in event logs
        val category = when (label) {
            0 -> LogType.RAPID_INSULIN
            1 -> LogType.CARBS
            2 -> LogType.BASAL_INSULIN
            3 -> LogType.BLOOD_GLUCOSE
            else -> null
        }
        for (log in _logs.value) {
            val categoryMatches = category == null || log.type == category ||
                (category == LogType.CARBS && log.type == LogType.MEAL)
            val keywordMatches = if (keyword.isNotEmpty()) {
                log.note.contains(keyword, ignoreCase = true) ||
                    log.labelName.contains(keyword, ignoreCase = true) ||
                    log.mealSummary.contains(keyword, ignoreCase = true) ||
                    typeLabels[log.type]?.contains(keyword, ignoreCase = true) == true
            } else true

            if (categoryMatches && keywordMatches) {
                matches.add(log.timestamp)
            }
        }

        try {
            if (Applic.Nativesloaded) {
                // Native searches by label code, not by the category picked here.
                Natives.search(category?.let(::nativeLabel) ?: -1, under, above, 0, 0, true, keyword, 0f)
            }
        } catch (_: Throwable) {}

        matches.sort()
        return matches.distinct()
    }

    fun nextSearchMatch() {
        scope.launch(Dispatchers.IO) {
            try {
                if (Applic.Nativesloaded) {
                    Natives.latersearch()
                }
            } catch (_: Throwable) {}
            refreshAll()
        }
    }

    fun prevSearchMatch() {
        scope.launch(Dispatchers.IO) {
            try {
                if (Applic.Nativesloaded) {
                    Natives.earliersearch()
                }
            } catch (_: Throwable) {}
            refreshAll()
        }
    }

    fun stopSearch() {
        scope.launch(Dispatchers.IO) {
            try {
                if (Applic.Nativesloaded) {
                    Natives.stopsearch()
                }
            } catch (_: Throwable) {}
            refreshAll()
        }
    }

    /**
     * Recomputes the derived statistics from the currently published readings.
     *
     * [readings] is sorted by timestamp, so both windows are located with a binary search and
     * addressed through [List.subList] views instead of building two throwaway filtered copies of a
     * list that can hold six figures of points. The AGP profile is memoised against the inputs that
     * actually change it, because rebuilding 24 percentile buckets over the whole history every time
     * a single reading arrives was by far the most expensive thing this class did.
     */
    private fun recalculateStats() {
        val all = _readings.value
        val period = _statsPeriod.value
        val range = _range.value
        val now = System.currentTimeMillis()

        if (all.isEmpty()) {
            _stats.value = GlucoseStats()
            _screenStats.value = GlucoseStats()
            _agpProfile.value = AgpProfile.calculate(emptyList(), period)
            publishedAgpKey = null
            return
        }

        val periodStart = lowerBound(all, now - period.durationMillis)
        val toUse = if (periodStart < all.size) all.subList(periodStart, all.size) else all

        _stats.value = GlucoseStats.calculate(toUse, range, period.durationMillis)

        val agpKey = agpCacheKey(toUse, period, range)
        if (agpKey != publishedAgpKey) {
            _agpProfile.value = AgpProfile.calculate(toUse, period)
            publishedAgpKey = agpKey
        }

        // Statistics for the selected screen range (e.g. 1h, 6h or a custom duration).
        val screenDuration = _selectedTimeRange.value?.durationMillis ?: (6 * 3600 * 1000L)
        val screenStart = lowerBound(all, now - screenDuration)
        val screenToUse = if (screenStart < all.size) all.subList(screenStart, all.size) else all
        _screenStats.value = GlucoseStats.calculate(screenToUse, range, screenDuration)
    }

    private data class AgpCacheKey(
        val period: StatsPeriod,
        val range: GlucoseRange,
        val count: Int,
        val firstTimestamp: Long,
        val lastTimestamp: Long,
        val valueDigest: Long
    )

    @Volatile
    private var publishedAgpKey: AgpCacheKey? = null

    private fun agpCacheKey(
        toUse: List<GlucosePoint>,
        period: StatsPeriod,
        range: GlucoseRange
    ): AgpCacheKey {
        var digest = 7L
        for (point in toUse) {
            digest = digest * 31 + point.valueMgDl.toRawBits()
        }
        return AgpCacheKey(
            period = period,
            range = range,
            count = toUse.size,
            firstTimestamp = toUse.firstOrNull()?.timestamp ?: 0L,
            lastTimestamp = toUse.lastOrNull()?.timestamp ?: 0L,
            valueDigest = digest
        )
    }

    /** Index of the first element of the timestamp-sorted [points] at or after [time]. */
    private fun lowerBound(points: List<GlucosePoint>, time: Long): Int {
        var low = 0
        var high = points.size
        while (low < high) {
            val mid = (low + high) ushr 1
            if (points[mid].timestamp < time) low = mid + 1 else high = mid
        }
        return low
    }

    /**
     * Heartbeat that keeps the UI live.
     *
     * The cheap part - a single [Natives.lastglucose] call - still runs every few seconds and
     * appends the point, which is all a live view needs. The expensive part used to run every 9
     * seconds and walked every raw, calibrated and scan record of every sensor: hundreds of
     * thousands of JNI calls, a fresh [GlucosePoint] each, a full sort, and tens of megabytes of
     * garbage that stalled the render thread with 40 ms GC pauses. That is now a slow safety net,
     * with the event-driven [refreshAll] covering the cases that actually matter (sensor activated,
     * calibration written, scan taken, Bluetooth state changed).
     */
    private fun startPolling() {
        scope.launch(Dispatchers.IO) {
            var ticks = 0
            while (isActive) {
                delay(FAST_POLL_INTERVAL_MILLIS)
                if (!uiVisible.value) {
                    uiVisible.first { it }
                    continue
                }
                try {
                    ticks++
                    if (Applic.Nativesloaded) {
                        val strGl = Natives.lastglucose()
                        if (strGl != null && strGl.time > 0) {
                            val rawVal = try {
                                strGl.value.replace(',', '.').toFloat()
                            } catch (_: Throwable) { 0f }
                            val valMgDl = if (_unit.value == GlucoseUnit.MMOL_L) {
                                GlucoseUnit.MMOL_L.toMgDl(rawVal)
                            } else rawVal

                            if (valMgDl > 0f) {
                                val newPt = GlucosePoint(
                                    timestamp = strGl.time * 1000L,
                                    valueMgDl = valMgDl,
                                    rate = strGl.rate,
                                    status = _range.value.statusOf(valMgDl)
                                )
                                // A new object with the same time and value would still be unequal to
                                // the old one, and that is enough to make every collector of
                                // `currentReading` recompose - the home screen's hero and sparkline
                                // included - three times a minute for no reason.
                                val previous = _currentReading.value
                                if (previous == null ||
                                    previous.timestamp != newPt.timestamp ||
                                    previous.valueMgDl != newPt.valueMgDl
                                ) {
                                    _currentReading.value = newPt
                                }
                                if (appendReading(newPt)) {
                                    requestStatsRecalculation()
                                }
                            }
                        }
                    }
                    if (ticks % FAST_TICKS_PER_SENSOR_SWEEP == 0) {
                        sweepSensors()
                    }
                    if (_insulinOnboardEnabled.value && ticks % FAST_TICKS_PER_IOB_REFRESH == 0) {
                        // Nothing on board changes on its own except by decaying, so this is the
                        // one thing that has to follow the clock rather than an event.
                        publishInsulinOnboard()
                    }
                    if (ticks % FAST_TICKS_PER_FULL_REFRESH == 0) {
                        refreshAllLocked()
                    }
                    if (ticks % FAST_TICKS_PER_DEVICE_SWEEP == 0) {
                        // The paired-watch list is a phone screen; a watch never shows it.
                        if (!Applic.isWearable) refreshWearDevices()
                        refreshMirrorConnections()
                    }
                } catch (_: Throwable) {}
            }
        }
    }

    /**
     * Sensor sweep that yields to a full reload rather than racing it. Both drive the same native
     * library, and two threads inside it at once is not something to find out about on a watch.
     */
    private fun sweepSensors() {
        if (!nativeLoadMutex.tryLock()) return
        try {
            loadSensorsFromNative()
        } finally {
            nativeLoadMutex.unlock()
        }
    }

    /**
     * Adds [point] to the published history when it is genuinely new. Returns whether the history
     * actually changed.
     *
     * `readings` is kept sorted, and the same reading arrives here as both a raw and a calibrated
     * point, so the insert has to be stable and the duplicate check has to key on the timestamp
     * alone. [publishReadings] then decides whether anything is worth emitting.
     */
    private fun appendReading(point: GlucosePoint): Boolean {
        val current = _readings.value
        val insertAt = lowerBound(current, point.timestamp)
        if (insertAt < current.size && current[insertAt].timestamp == point.timestamp) return false
        val next = ArrayList<GlucosePoint>(current.size + 1)
        next.addAll(current.subList(0, insertAt))
        next.add(point)
        next.addAll(current.subList(insertAt, current.size))
        val before = _readings.value
        publishReadings(next)
        return before !== _readings.value
    }

    private fun isNfcLaunchEnabled(): Boolean {
        return try {
            val component = ComponentName(Applic.app, NFC_LAUNCH_COMPONENT)
            Applic.app.packageManager.getComponentEnabledSetting(component) !=
                PackageManager.COMPONENT_ENABLED_STATE_DISABLED
        } catch (_: Throwable) {
            true
        }
    }

    private companion object {
        const val NFC_LAUNCH_COMPONENT = "tk.glucodata.glucodata"
        const val MIRROR_MAX_ADDRESSES = 4
        // Taken from the native storage sizes, so a draft can never be built that the
        // native side would reject after the editor already accepted it.
        const val MIRROR_LABEL_MAX_LENGTH = Natives.MAXMIRRORLABELLENGTH
        const val MIRROR_PASSWORD_MAX_LENGTH = Natives.MAXMIRRORPASSLENGTH
        const val MIRROR_ICE_LABEL_MIN_LENGTH = 16
        const val MIRROR_ICE_LABEL_MAX_LENGTH = 32
        const val MIRROR_HOSTNAME_MAX_LENGTH = 81
        const val MIRROR_NOW_SECONDS = 300L
        const val MIRROR_CODE_SUFFIX = "MirrorJuggluco"

        /**
         * The `night` index of the native treatment mappings: 0 is the LibreView export, 1 the
         * Nightscout one. Only the LibreView half belongs to this screen.
         */
        const val LIBRE_NIGHT = 0

        /**
         * Limits of the fixed size credential fields in the settings struct (`char libreemail[256]`,
         * `char librepass[36]`). Native copies straight into them without a bounds check, so
         * anything longer has to be refused before it gets there.
         */
        const val LIBRE_EMAIL_MIN_LENGTH = 3
        const val LIBRE_EMAIL_MAX_LENGTH = 255
        const val LIBRE_PASSWORD_MIN_LENGTH = 3
        const val LIBRE_PASSWORD_MAX_LENGTH = 36

        /**
         * Connection codes are a phone feature: the native side does not export
         * them on Wear OS, so the editor hides everything that needs one. Probed lazily:
         * the class loads before native networking has started.
         */
        val mirrorCodesAvailable: Boolean by lazy {
            try {
                Natives.getbackJson(-1)
                true
            } catch (_: Throwable) {
                false
            }
        }
        const val CALIBRATION_PREFS = "calibration_prefs"
        const val KEY_CALIBRATION_PROMPT_SHOWN = "calibration_prompt_shown"
        const val UI_PREFS = "ui_prefs"
        const val KEY_DELTA_CALCULATION = "delta_calculation_minutes"
        const val KEY_MINIMALIST_UNITS = "minimalist_units"
        const val KEY_SHOW_ALERT_LINES = "show_alert_lines"
        /** The "Blood" label of the native default label set. */
        const val DEFAULT_BLOOD_LABEL = 6
        const val LOG_NOTES_PREFS = "log_notes"
        const val LOG_NOTES_KEY = "notes"
        /** Native alarm kinds with user-facing sound behavior, in UI order. */
        val behaviorKinds = listOf(0, 5, 1, 6, 7, 8, 4, 2)

        /**
         * Heartbeat cadence. The cheap `lastglucose` poll stays at 3 s because a new reading really
         * does arrive every 5 minutes and a user watching the number expects it to move promptly.
         */
        const val FAST_POLL_INTERVAL_MILLIS = 3_000L

        /** Sensor list sweep, 15 s: cheap, and only publishes when something actually differs. */
        const val FAST_TICKS_PER_SENSOR_SWEEP = 5

        /** Paired-device sweep, 30 s. */
        const val FAST_TICKS_PER_DEVICE_SWEEP = 10

        /**
         * Insulin on board refresh, 30 s. Doses decay over hours rather than minutes, and the
         * value is rounded to a tenth, so a minute apart would not read any differently.
         */
        const val FAST_TICKS_PER_IOB_REFRESH = 10

        /**
         * Full native reload, 2 minutes. This walks every raw, calibrated and scan record of every
         * sensor - six figures of JNI calls and tens of megabytes of garbage - so it is a safety net
         * for "something changed and nobody told us", not the primary update path. Sensor
         * activation, calibration, scans and Bluetooth transitions all arrive through [refreshAll].
         */
        const val FAST_TICKS_PER_FULL_REFRESH = 40

        /** Pre-sizing hint for a full sensor read; grows on its own if a sensor holds more. */
        const val INITIAL_READING_CAPACITY = 4096

        val MARKUP_PATTERN = Regex("<[^>]+>")
    }
}
