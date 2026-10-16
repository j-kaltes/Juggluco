/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2(+), Libre 3(+), Dexcom G7/ONE+,        */
/*      Sibionics GS1Sb and GS3, Accu-Chek SmartGuide, CareSens Air and              */
/*      Aidex X sensors.                                                             */
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
/*      Thu Oct 15 12:44:39 CEST 2026                                                */

// SPDX-License-Identifier: GPL-3.0-or-later
package tk.glucodata;

import android.app.Activity;
import android.app.Application;
import android.app.Fragment;
import android.app.FragmentManager;
import android.bluetooth.BluetoothManager;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.ComponentCallbacks;
import android.content.res.Configuration;
import android.content.res.TypedArray;
import android.content.Intent;
import android.content.ActivityNotFoundException;
import android.content.SharedPreferences;
import android.graphics.Typeface;
import android.graphics.Rect;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.GradientDrawable;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.PersistableBundle;
import android.os.ParcelFileDescriptor;
import android.os.PowerManager;
import android.text.Editable;
import android.text.InputType;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.text.TextPaint;
import android.text.method.LinkMovementMethod;
import android.text.style.ClickableSpan;
import android.text.style.StyleSpan;
import android.text.style.RelativeSizeSpan;
import android.text.style.TypefaceSpan;
import android.text.style.QuoteSpan;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.view.Window;
import android.view.WindowManager;
import android.view.inputmethod.EditorInfo;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.PopupWindow;
import android.widget.Toast;
import android.widget.Button;
import android.widget.TextView;
import android.webkit.WebView;
import android.webkit.WebViewClient;

import androidx.annotation.Keep;
import androidx.activity.OnBackPressedCallback;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatDialog;
import androidx.appcompat.widget.AppCompatButton;
import androidx.appcompat.widget.AppCompatEditText;
import androidx.appcompat.widget.AppCompatTextView;
import androidx.core.graphics.ColorUtils;
import androidx.core.content.FileProvider;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.core.widget.NestedScrollView;
import com.google.android.gms.wearable.Node;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.lang.ref.WeakReference;
import java.io.IOException;
import java.io.File;
import java.io.FileInputStream;
import java.io.ByteArrayOutputStream;
import java.nio.charset.StandardCharsets;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/** Android UI only. Authentication, model communication and CGM access are native. */
@Keep
public final class JugglucoChat {
    private static final long POLL_MS = 400L;
    private static final long WAKE_LIMIT_MS = 15L * 60L * 1000L;
    private static WeakReference<ChatWindow> current = new WeakReference<>(null);
    private static RunController running;

    private JugglucoChat() {}

    private static native NativeMessage nativeInitWithFilesMessages(String storageDir, String filesDir);
    private static native NativeMessage nativeStartLoginMessages();
    private static native NativeMessage nativeLoadModelsMessages();
    private static native NativeMessage nativeSendWithOptionsMessages(String question, String model, String reasoningEffort, boolean internet);
    private static native NativeMessage nativeCancelMessages();
    private static native NativeMessage nativeNewChatMessages();
    private static native NativeMessage nativeClearAnalysisMessages();
    private static native NativeMessage nativeLogoutMessages();
    private static native String nativePollMessages();
    private static native String nativePollWorkMessages();
    private static native NativeMessage nativeSetWearNodesMessages(String snapshot);
    private static native NativeMessage nativeSetPhoneActivityMessages(String snapshot);
    private static native String nativeGetPlot(int message, int plot);
    private static native String nativeListFilesMessages();
    private static native String nativeFileUrlMessages(String id, String file);
    private static native String nativeTakeBrowserFile();
    private static native NativeMessage nativeExportFileMessages(String filesDir, String id, String file, int descriptor);
    private static native NativeMessage nativeWriteChatExportWithStringsMessages(String format, int descriptor, String labels);
    private static native NativeMessage nativeWritePlotExportMessages(String svg, int descriptor);
    private static native NativeMessage nativeCopyChatExportMessages(int input, int output);

    /** The manifest grants access only to the export cache, never app files. */
    @Keep
    public static final class ExportProvider extends FileProvider {
        public ExportProvider() { super(); }
    }

    /** Own the picker independently of the chat dialog. Android restores the
     * fragment/arguments and routes its result after Activity/process recreation.
     * Retaining it preserves an in-progress copy across configuration changes.
     * Uses the framework Fragment already available on all supported phones;
     * no MainActivity result hook or extra Android dependency is needed. */
    @Keep
    @SuppressWarnings("deprecation")
    public static final class SaveFileFragment extends Fragment {
        static final String TAG = "JugglucoChat.SaveFile";
        static final int CREATE_DOCUMENT = 1;
        boolean launched, preparing, writing, finished, delivered, resultError;
        String resultText = "", exportPath = "";

        private Context textContext;
        String s(int id, Object... args) { return textContext.getString(id, args); }
        public SaveFileFragment() {}

        @Override public void onCreate(Bundle state) {
            super.onCreate(state);
            textContext = getActivity().getApplicationContext();
            setRetainInstance(true);
            if (state != null) {
                launched = state.getBoolean("launched");
                finished = state.getBoolean("finished");
                resultError = state.getBoolean("resultError");
                resultText = state.getString("resultText", "");
                exportPath = state.getString("exportPath", "");
                if (state.getBoolean("preparing") && !finished) {
                    // Do not accidentally export a different chat after a
                    // process restart during snapshot preparation.
                    finished = resultError = true;
                    resultText = s(R.string.jgchat_preparing_the_export_was_interrupted_try_save_or_share_again);
                }
                if (state.getBoolean("writing") && !finished) {
                    // A new process cannot resume an old provider descriptor.
                    finished = resultError = true;
                    resultText = s(R.string.jgchat_saving_was_interrupted_try_save_as_again_a_partial_copy_may_remain_at_the_selected_loc);
                }
            }
        }
        @Override public void onSaveInstanceState(Bundle state) {
            super.onSaveInstanceState(state);
            state.putBoolean("launched", launched);
            state.putBoolean("writing", writing);
            state.putBoolean("preparing", preparing);
            state.putString("exportPath", exportPath);
            state.putBoolean("finished", finished);
            state.putBoolean("resultError", resultError);
            state.putString("resultText", resultText);
        }
        @Override public void onStart() {
            super.onStart();
            if (getArguments().containsKey("format") && exportPath.isEmpty() && !finished) {
                if (!preparing) prepareExport();
            } else launchPicker();
        }
        void launchPicker() {
            if (!isResumed() || launched || preparing || finished) return;
            launched = true;
            try {
                String name = exportPath.isEmpty() ? getArguments().getString("file") : new File(exportPath).getName();
                if (getArguments().getBoolean("share")) {
                    File file = exportFile(getActivity(), exportPath);
                    Uri uri = FileProvider.getUriForFile(getActivity(),
                            getActivity().getPackageName() + ".chat.exports", file);
                    Intent intent = new Intent(Intent.ACTION_SEND).setType(documentType(name))
                            .putExtra(Intent.EXTRA_STREAM, uri).putExtra(Intent.EXTRA_TITLE, name)
                            .putExtra(Intent.EXTRA_SUBJECT, name)
                            .addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
                    // ClipData also propagates read grants on older Android.
                    intent.setClipData(ClipData.newRawUri(name, uri));
                    // Text-only receivers often use EXTRA_TEXT. Always retain
                    // the full attachment; avoid oversized Binder transactions.
                    if (name.endsWith(".txt") && file.length() <= 65536) {
                        try (FileInputStream input = new FileInputStream(file)) {
                            ByteArrayOutputStream contents = new ByteArrayOutputStream();
                            byte[] buffer = new byte[4096]; int count;
                            while ((count = input.read(buffer)) != -1) contents.write(buffer, 0, count);
                            intent.putExtra(Intent.EXTRA_TEXT, new String(contents.toByteArray(), StandardCharsets.UTF_8));
                        }
                    }
                    startActivity(Intent.createChooser(intent, (name.endsWith(".svg") ? s(R.string.jgchat_share_plot) : s(R.string.jgchat_share_chat))));
                    complete("", false);
                    return;
                }
                Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT)
                        .addCategory(Intent.CATEGORY_OPENABLE).setType(documentType(name))
                        .putExtra(Intent.EXTRA_TITLE, name);
                startActivityForResult(intent, CREATE_DOCUMENT);
            } catch (ActivityNotFoundException failure) {
                complete(getArguments().getBoolean("share")
                        ? s(R.string.jgchat_no_installed_app_can_receive_this_file_format_try_save_instead)
                        : s(R.string.jgchat_no_android_document_picker_is_available), true);
            } catch (IOException failure) {
                complete(s(R.string.jgchat_the_prepared_export_is_no_longer_available_try_save_or_share_again), true);
            } catch (RuntimeException failure) {
                complete(getArguments().getBoolean("share")
                        ? s(R.string.jgchat_could_not_share_the_export_check_that_the_chat_export_provider_and_xml_resource_are_in)
                        : s(R.string.jgchat_could_not_open_android_s_save_as_screen), true);
            }
        }
        @Override public void onResume() { super.onResume(); launchPicker(); deliverResult(); }
        void prepareExport() {
            preparing = true;
            final Context app = getActivity().getApplicationContext();
            final String format = getArguments().getString("format");
            final String svg = getArguments().getString("svg");
            Toast.makeText(app, s(R.string.jgchat_preparing_export), Toast.LENGTH_SHORT).show();
            new Thread(() -> {
                File file = null;
                String error = "";
                try {
                    if (!"html".equals(format) && !"txt".equals(format) && !"svg".equals(format))
                        throw new IOException(s(R.string.jgchat_unsupported_format));
                    File directory = new File(app.getCacheDir(), "juggluco-chat-exports");
                    if (!directory.isDirectory() && !directory.mkdirs()) throw new IOException(s(R.string.jgchat_cannot_create_export_cache));
                    File[] old = directory.listFiles();
                    long cutoff = System.currentTimeMillis() - 7L * 24L * 60L * 60L * 1000L;
                    if (old != null) for (File entry : old)
                        if (entry.isFile() && entry.getName().startsWith("Juggluco-") && entry.lastModified() < cutoff) entry.delete();
                    String date = new SimpleDateFormat("yyyyMMdd-HHmmss", Locale.ROOT).format(new Date());
                    file = File.createTempFile("Juggluco-" + ("svg".equals(format) ? "plot-" : "chat-") + date + "-", "." + format, directory);
                    try (ParcelFileDescriptor output = ParcelFileDescriptor.open(file,
                            ParcelFileDescriptor.MODE_WRITE_ONLY | ParcelFileDescriptor.MODE_TRUNCATE)) {
                        NativeMessage result = "svg".equals(format) ? nativeWritePlotExportMessages(svg, output.getFd())
                                : nativeWriteChatExportWithStringsMessages(format, output.getFd(), NativeMessage.exportLabels(app));
                        if (result != null) error = result.text(app);
                    }
                } catch (Exception | LinkageError failure) { error = s(R.string.jgchat_could_not_prepare_the_export_try_again); }
                final File prepared = file;
                final String failure = error;
                new Handler(Looper.getMainLooper()).post(() -> {
                    preparing = false;
                    if (!failure.isEmpty() || prepared == null) {
                        if (prepared != null) prepared.delete();
                        complete(failure.isEmpty() ? s(R.string.jgchat_could_not_prepare_the_export) : failure, true);
                    } else {
                        exportPath = prepared.getAbsolutePath();
                        launchPicker();
                    }
                });
            }, "JugglucoChat-export").start();
        }
        // Arguments survive Android process recreation. Still require the
        // exact private export directory before opening a persisted path.
        static File exportFile(Context context, String path) throws IOException {
            File root = new File(context.getCacheDir(), "juggluco-chat-exports").getCanonicalFile();
            File file = new File(path).getCanonicalFile();
            if (!root.equals(file.getParentFile()) || !file.getName().startsWith("Juggluco-") || !file.isFile())
                throw new IOException(context.getString(R.string.jgchat_export_snapshot_is_unavailable));
            return file;
        }
        @Override public void onActivityResult(int requestCode, int resultCode, Intent data) {
            super.onActivityResult(requestCode, resultCode, data);
            if (requestCode != CREATE_DOCUMENT || writing || finished) return;
            if (resultCode != Activity.RESULT_OK) { complete("", false); return; }
            final Uri uri = data == null ? null : data.getData();
            if (uri == null || !"content".equals(uri.getScheme()) || getActivity() == null) {
                complete(s(R.string.jgchat_android_did_not_return_a_writable_document), true); return;
            }
            final Context app = getActivity().getApplicationContext();
            final String id = getArguments().getString("id"), file = getArguments().getString("file");
            final String snapshot = exportPath;
            writing = true;
            Toast.makeText(app, s(R.string.jgchat_saving_file), Toast.LENGTH_SHORT).show();
            new Thread(() -> {
                boolean failed = false;
                try (ParcelFileDescriptor document = app.getContentResolver().openFileDescriptor(uri, "wt")) {
                    if (document == null) throw new IOException(s(R.string.jgchat_no_document_descriptor));
                    NativeMessage error;
                    if (snapshot.isEmpty()) error = nativeExportFileMessages(app.getFilesDir().getAbsolutePath(), id, file, document.getFd());
                    else try (ParcelFileDescriptor input = ParcelFileDescriptor.open(exportFile(app, snapshot), ParcelFileDescriptor.MODE_READ_ONLY)) {
                        error = nativeCopyChatExportMessages(input.getFd(), document.getFd());
                    }
                    if (error != null) {
                        try { document.closeWithError(s(R.string.jgchat_could_not_copy_generated_file)); } catch (IOException ignored) {}
                        throw new IOException(s(R.string.jgchat_native_export_failed));
                    }
                    document.checkError();
                } catch (Exception | LinkageError failure) {
                    // Provider messages may contain private locations. Keep them
                    // out of chat/model data and diagnostic logs.
                    failed = true;
                }
                final boolean failedCopy = failed;
                new Handler(Looper.getMainLooper()).post(() -> complete(failedCopy
                        ? (snapshot.isEmpty() ? s(R.string.jgchat_could_not_save_the_file_the_original_is_still_in_saved_files_an_empty_or_partial_copy_)
                            : s(R.string.jgchat_could_not_save_the_export_try_save_chat_or_save_plot_again_an_empty_or_partial_copy_ma))
                        : s(R.string.jgchat_file_saved), failedCopy));
            }, "JugglucoChat-save").start();
        }
        void complete(String message, boolean error) {
            writing = false; finished = true; resultText = message; resultError = error;
            deliverResult();
        }
        void deliverResult() {
            if (!finished || delivered || !isResumed() || getActivity() == null) return;
            delivered = true;
            if (!resultText.isEmpty()) {
                ChatWindow chat = current.get();
                if (resultError && chat != null && !chat.closed && chat.activity == getActivity()) {
                    chat.localError = resultText; chat.showError();
                } else Toast.makeText(getActivity(), resultText, Toast.LENGTH_LONG).show();
            }
            getFragmentManager().beginTransaction().remove(this).commitAllowingStateLoss();
        }
    }

    private static String documentType(String name) {
        if (name.endsWith(".svg")) return "image/svg+xml";
        if (name.endsWith(".html") || name.endsWith(".htm")) return "text/html";
        if (name.endsWith(".csv")) return "text/csv";
        if (name.endsWith(".tsv")) return "text/tab-separated-values";
        if (name.endsWith(".json")) return "application/json";
        if (name.endsWith(".js") || name.endsWith(".mjs")) return "text/javascript";
        if (name.endsWith(".css")) return "text/css";
        if (name.endsWith(".md")) return "text/markdown";
        return "text/plain";
    }

    public static void show(Activity activity) {
        if (Looper.myLooper() != Looper.getMainLooper()) {
            activity.runOnUiThread(() -> show(activity));
            return;
        }
        if (activity.isFinishing() || (Build.VERSION.SDK_INT >= 17 && activity.isDestroyed())) return;
        if (running == null) running = new RunController(activity.getApplication());
        running.hostClass = activity.getClass();
        ChatWindow previous = current.get();
        if (previous != null && !previous.closed) {
            if (previous.activity == activity) {
                previous.refresh();
                return;
            }
            previous.dialog.dismiss();
        }
        ChatWindow window = new ChatWindow(activity);
        current = new WeakReference<>(window);
        window.open();
    }

    private interface NativeCall { NativeMessage run(); }

    /** Application-owned observation and wake lease. Closing a window only
     * detaches its views; it never owns or cancels the native request. All
     * methods run on the main thread and Activity/View references are weak. */
    private static final class RunController implements Application.ActivityLifecycleCallbacks {
        final Application app;
        final Handler handler = new Handler(Looper.getMainLooper());
        final Runnable pollTask = this::poll;
        PowerManager.WakeLock wakeLock;
        long observedOperation = -1, completedOperation = -1;
        long wearObservedAt;
        boolean busy, initialized, polling, pollAgain, settingsOpen;
        boolean reopenAfterRotation, restoreKeyboard;
        String draft = "";
        int selectionStart, selectionEnd, scrollY;
        boolean followTail = true;
        Class<?> hostClass;

        RunController(Application app) {
            this.app = app;
            app.registerActivityLifecycleCallbacks(this);
        }
        String s(int id, Object... args) { return app.getString(id, args); }
        boolean visible() {
            ChatWindow chat = current.get();
            return chat != null && !chat.closed && chat.resumed && chat.initialized;
        }
        void refresh() {
            if (!initialized) return;
            if (polling) { pollAgain = true; return; }
            handler.removeCallbacks(pollTask);
            poll();
        }
        void poll() {
            if (!initialized || polling) return;
            polling = true;
            try {
                long now = android.os.SystemClock.elapsedRealtime();
                if (wearObservedAt == 0 || now - wearObservedAt >= 5000) {
                    wearObservedAt = now;
                    publishWearNodes();
                    publishPhoneActivity();
                }
                boolean full = visible();
                String raw = full ? nativePollMessages() : nativePollWorkMessages();
                if (raw == null) throw new JSONException(s(R.string.jgchat_no_chat_status));
                JSONObject state = new JSONObject(raw);
                busy = state.optBoolean("busy", false);
                long operation = state.optLong("operation_id", 0);
                if (operation != observedOperation) {
                    releaseWake();
                    observedOperation = operation;
                    long remaining = WAKE_LIMIT_MS - state.optLong("elapsed_ms", 0);
                    if (busy && remaining > 0) {
                        try {
                            PowerManager power = (PowerManager) app.getSystemService(Context.POWER_SERVICE);
                            if (power != null) {
                                wakeLock = power.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "Juggluco:NativeChat");
                                wakeLock.setReferenceCounted(false);
                                wakeLock.acquire(Math.min(remaining, WAKE_LIMIT_MS));
                            }
                        } catch (RuntimeException unavailable) { releaseWake(); }
                    }
                }
                if (!busy) {
                    releaseWake();
                    if (operation != completedOperation) {
                        completedOperation = operation;
                        String unanswered = state.optString("pending_question", "");
                        if (draft.isEmpty() && !unanswered.isEmpty()) {
                            draft = unanswered;
                            selectionStart = selectionEnd = draft.length();
                        }
                    }
                }
                ChatWindow chat = current.get();
                if (full && chat != null && !chat.closed) chat.apply(state);
            } catch (JSONException | RuntimeException | LinkageError failure) {
                ChatWindow chat = current.get();
                if (chat != null && !chat.closed) {
                    chat.localError = s(R.string.jgchat_could_not_update_chat_status_check_that_java_and_native_code_were_both_updated);
                    chat.showError();
                }
            } finally {
                polling = false;
                handler.removeCallbacks(pollTask);
                if (pollAgain) { pollAgain = false; handler.post(pollTask); }
                else if (busy || visible()) handler.postDelayed(pollTask, POLL_MS);
            }
        }
        static Object seconds(long millis) { return millis > 0 ? millis / 1000L : JSONObject.NULL; }
        static Object textOrNull(String value) {
            return value == null || value.isEmpty() ? JSONObject.NULL : value.substring(0, Math.min(512, value.length()));
        }
        JSONObject sensorActivity() throws JSONException {
            JSONObject out = new JSONObject().put("status", "unavailable");
            try {
                BluetoothManager manager = (BluetoothManager) app.getSystemService(Context.BLUETOOTH_SERVICE);
                out.put("bluetooth_enabled", manager == null || manager.getAdapter() == null
                        ? JSONObject.NULL : manager.getAdapter().isEnabled());
            } catch (RuntimeException missingPermission) { out.put("bluetooth_enabled", JSONObject.NULL); }
            SensorBluetooth source = SensorBluetooth.blueone;
            if (source == null) return out;
            out.put("scan_started_at", seconds(source.scantime)).put("scan_timeout_at", seconds(source.scantimeouttime))
                    .put("scan_stopped_at", seconds(source.stopscantime));
            ArrayList<SuperGattCallback> callbacks = SensorBluetooth.mygatts();
            // Copy references before iterating; do not call JNI with a sensor
            // pointer that a Bluetooth callback could release concurrently.
            SuperGattCallback[] copy = callbacks == null ? new SuperGattCallback[0]
                    : callbacks.toArray(new SuperGattCallback[0]);
            JSONArray rows = new JSONArray();
            for (SuperGattCallback gatt : copy) {
                if (rows.length() == 64) break;
                if (gatt == null) continue;
                rows.put(new JSONObject().put("sensor_id", textOrNull(gatt.SerialNumber))
                        .put("protocol_generation", gatt.sensorgen).put("stopped", gatt.stop)
                        .put("address_known", gatt.mActiveDeviceAddress != null && !gatt.mActiveDeviceAddress.isEmpty())
                        .put("started_at", seconds(gatt.starttime)).put("connection_attempt_at", seconds(gatt.connectTime))
                        .put("found_at", seconds(gatt.foundtime)).put("connected_at", seconds(gatt.constatchange[0]))
                        .put("disconnected_at", seconds(gatt.constatchange[1]))
                        .put("connection_status", textOrNull(gatt.constatstatusstr))
                        .put("handshake_status", textOrNull(gatt.handshake))
                        .put("handshake_success_at", seconds(gatt.wrotepass[0]))
                        .put("handshake_failure_at", seconds(gatt.wrotepass[1]))
                        .put("glucose_success_at", seconds(gatt.charcha[0])).put("glucose_failure_at", seconds(gatt.charcha[1]))
                        .put("rssi_dbm", gatt.readrssi < 0 ? gatt.readrssi : JSONObject.NULL));
            }
            return out.put("status", "ok").put("total", copy.length).put("truncated", copy.length > rows.length())
                    .put("callbacks", rows);
        }
        JSONObject garminActivity() throws JSONException {
            JSONObject out = new JSONObject().put("status", "unavailable");
            if (Applic.app == null || Applic.app.numdata == null) return out;
            tk.glucodata.nums.AllData data = Applic.app.numdata;
            List<tk.glucodata.nums.AllData.GarminDeviceInfo> devices = data.getGarminDeviceInfos();
            JSONArray rows = new JSONArray();
            for (tk.glucodata.nums.AllData.GarminDeviceInfo watch : devices) {
                if (rows.length() == 64) break;
                rows.put(new JSONObject().put("id", Long.toString(watch.id)).put("name", textOrNull(watch.name))
                        .put("connected", watch.connected).put("direct_ble", watch.direct).put("active", watch.active)
                        .put("send_glucose", watch.glucose).put("numbers_device", watch.numbers)
                        .put("libre3_direct", watch.libre3Direct).put("libre3_installed", watch.libre3Installed)
                        .put("watch_stopped", watch.watchStopped).put("timestamped_glucose_ack", watch.timestampedGlucoseAck)
                        .put("app_version", watch.appVersion).put("communication_status", textOrNull(watch.communicationStatus))
                        .put("last_sent_at", seconds(watch.lastSend)).put("last_received_at", seconds(watch.lastReceived))
                        .put("last_status_at", seconds(watch.lastStatusTime))
                        .put("last_transport_status", watch.lastStatus == null ? JSONObject.NULL : watch.lastStatus.name())
                        .put("last_acknowledged_at", seconds(watch.lastAcknowledged))
                        .put("last_glucose_acknowledged_at", seconds(watch.lastGlucoseAcknowledged))
                        .put("acknowledged_glucose_sample_at", watch.acknowledgedGlucoseTime > 0
                                ? watch.acknowledgedGlucoseTime : JSONObject.NULL)
                        .put("last_error", textOrNull(watch.lastError)));
            }
            return out.put("status", "ok").put("has_active_watch", data.usewatch)
                    .put("transport_mode", data.getGarminTransportMode(app))
                    .put("total", devices.size()).put("truncated", devices.size() > rows.length()).put("watches", rows);
        }
        void publishPhoneActivity() {
            try {
                JSONObject sensor, garmin;
                try { sensor = sensorActivity(); }
                catch (RuntimeException | LinkageError unavailable) {
                    sensor = new JSONObject().put("status", "unavailable");
                }
                try { garmin = garminActivity(); }
                catch (RuntimeException | LinkageError unavailable) {
                    garmin = new JSONObject().put("status", "unavailable");
                }
                NativeMessage error = nativeSetPhoneActivityMessages(new JSONObject().put("sensors", sensor).put("garmin", garmin).toString());
                if (error != null) Log.i("JugglucoChat", "Phone activity snapshot rejected");
            } catch (JSONException | RuntimeException | LinkageError unavailable) {
                Log.i("JugglucoChat", "Phone activity snapshot unavailable");
            }
        }
        void publishWearNodes() {
            try {
                JSONArray nodes = new JSONArray();
                MessageSender sender = MessageSender.getMessageSender();
                java.util.Set<Node> cached = sender == null ? null : sender.getNodes();
                if (cached != null) for (Node node : cached) {
                    if (nodes.length() == 64) break;
                    nodes.put(new JSONObject().put("id", node.getId()).put("name", node.getDisplayName())
                            .put("nearby", node.isNearby()));
                }
                nativeSetWearNodesMessages(new JSONObject().put("available", cached != null).put("nodes", nodes).toString());
            } catch (JSONException | RuntimeException | LinkageError unavailable) {
                // Optional Android cache only. Never initiate discovery or stop
                // a question if Google services / this cache are unavailable.
            }
        }
        void releaseWake() {
            PowerManager.WakeLock held = wakeLock; wakeLock = null;
            if (held != null) try { if (held.isHeld()) held.release(); } catch (RuntimeException ignored) {}
        }
        @Override public void onActivityResumed(Activity activity) {
            if (activity.getClass() != hostClass) return;
            if (reopenAfterRotation) {
                reopenAfterRotation = false;
                handler.post(() -> show(activity));
            }
            refresh();
        }
        @Override public void onActivityDestroyed(Activity activity) {}
        @Override public void onActivityCreated(Activity a, Bundle state) {}
        @Override public void onActivityStarted(Activity a) {}
        @Override public void onActivityPaused(Activity a) {}
        @Override public void onActivityStopped(Activity a) {}
        @Override public void onActivitySaveInstanceState(Activity a, Bundle state) {}
    }

    static String answerHeading(String modelName, String modelId, String fallback) {
        if (modelName != null && !modelName.trim().isEmpty()) return modelName.trim();
        if (modelId != null && !modelId.trim().isEmpty()) return modelId.trim();
        // Earlier releases did not record which model answered a message.
        return fallback;
    }

    /** Position in physical screen coordinates, including the keyboard area. */
    static int keyboardButtonTop(int keyboardTop, int height, boolean portrait, int top, int bottom) {
        return Math.max(top, Math.min(keyboardTop - (portrait ? 0 : height), bottom - height));
    }

    /** Horizontal popup offset in its parent window; bounds are screen coordinates. */
    static int keyboardButtonLeft(int width, boolean rtl, int left, int right, int margin, int parentLeft) {
        int lastLeft = Math.max(left, right - width);
        int wanted = rtl ? left + margin : lastLeft - margin;
        return Math.max(left, Math.min(wanted, lastLeft)) - parentLeft;
    }

    static int reasoningAbbreviation(String effort) {
        if (effort == null) return 0; // Older answers did not record the level.
        switch (effort) {
            case "": case "default": return R.string.jgchat_reasoning_default_short;
            case "none": return R.string.jgchat_reasoning_none_short;
            case "minimal": return R.string.jgchat_reasoning_minimal_short;
            case "low": return R.string.jgchat_reasoning_low_short;
            case "medium": return R.string.jgchat_reasoning_medium_short;
            case "high": return R.string.jgchat_reasoning_high_short;
            case "xhigh": return R.string.jgchat_reasoning_xhigh_short;
            case "max": return R.string.jgchat_reasoning_max_short;
            default: return 0;
        }
    }

    /** Small, non-HTML formatter. Offsets map original UTF-16 positions to the
     * displayed text so source citations survive removed Markdown delimiters.
     * Kept independent of Android text classes for host-side regression tests. */
    static final class Markup {
        static final int BOLD = 1, ITALIC = 2, BOTH = 3, CODE = 4, HEADING = 5, QUOTE = 6, LINK = 7;
        static final class Mark {
            final int kind; int start, end; final String url;
            Mark(int kind, int start, int end, String url) { this.kind = kind; this.start = start; this.end = end; this.url = url; }
        }
        static final class Result {
            final String text; final int[] positions; final List<Mark> marks;
            Result(String text, int[] positions, List<Mark> marks) { this.text = text; this.positions = positions; this.marks = marks; }
        }
        static void range(boolean[] flags, int start, int end) {
            for (int i = start; i < end; ++i) flags[i] = true;
        }
        static Result parse(String source) {
            final int size = source.length();
            boolean[] remove = new boolean[size], code = new boolean[size], escaped = new boolean[size];
            char[] chars = source.toCharArray();
            List<Mark> marks = new ArrayList<>();
            Matcher fences = Pattern.compile("(?m)^ {0,3}(`{3,}|~{3,})[^\\n]*\\n").matcher(source);
            while (fences.find()) {
                if (code[fences.start()]) continue;
                String token = fences.group(1);
                Matcher close = Pattern.compile("(?m)^ {0,3}" + Pattern.quote(token) + "[ \\t]*(?:\\n|$)").matcher(source);
                int bodyEnd = size, blockEnd = size;
                if (close.find(fences.end())) { bodyEnd = close.start(); blockEnd = close.end(); }
                range(code, fences.start(), blockEnd);
                range(remove, fences.start(), fences.end());
                range(remove, bodyEnd, blockEnd);
                marks.add(new Mark(CODE, fences.end(), bodyEnd, null));
            }
            for (int i = 0; i + 1 < size; ++i) {
                if (!code[i] && chars[i] == '\\' && "\\`*_{}[]()#+-.!>".indexOf(chars[i + 1]) >= 0) {
                    remove[i] = true; escaped[++i] = true;
                }
            }
            Matcher inline = Pattern.compile("(`+)([^`\\n]+)\\1").matcher(source);
            while (inline.find()) {
                if (code[inline.start()] || escaped[inline.start()]) continue;
                range(code, inline.start(), inline.end());
                range(remove, inline.start(), inline.start(2));
                range(remove, inline.end(2), inline.end());
                marks.add(new Mark(CODE, inline.start(2), inline.end(2), null));
            }
            Matcher lines = Pattern.compile("(?m)^ {0,3}(#{1,6}[ \\t]+|>[ \\t]?|[-+*][ \\t]+)([^\\n]+)").matcher(source);
            while (lines.find()) {
                if (code[lines.start()] || escaped[lines.start(1)]) continue;
                char marker = source.charAt(lines.start(1));
                if (marker == '#' || marker == '>') {
                    range(remove, lines.start(), lines.start(2));
                    marks.add(new Mark(marker == '#' ? HEADING : QUOTE, lines.start(2), lines.end(2), null));
                } else chars[lines.start(1)] = '•';
            }
            Matcher links = Pattern.compile("\\[([^]\\n]+)\\]\\((https?://[^\\s)]+)\\)").matcher(source);
            while (links.find()) {
                if (code[links.start()] || escaped[links.start()]) continue;
                range(remove, links.start(), links.start(1));
                range(remove, links.end(1), links.end());
                if (links.start() > 0 && chars[links.start() - 1] == '!' && !escaped[links.start() - 1]) remove[links.start() - 1] = true;
                marks.add(new Mark(LINK, links.start(1), links.end(1), links.group(2)));
            }
            String[] patterns = {
                "(?<![\\\\*])\\*\\*\\*(?=\\S)([^*\\n]+?)\\*\\*\\*(?!\\*)",
                "(?<![\\\\*])\\*\\*(?=\\S)([^\\n]+?)\\*\\*(?!\\*)",
                "(?<![\\\\\\w_])__(?=\\S)([^_\\n]+?)__(?![\\w_])",
                "(?<![\\\\*])\\*(?=\\S)([^*\\n]+?)\\*(?!\\*)",
                "(?<![\\\\\\w_])_(?=\\S)([^_\\n]+?)_(?![\\w_])"
            };
            int[] kinds = {BOTH, BOLD, BOLD, ITALIC, ITALIC};
            for (int p = 0; p < patterns.length; ++p) {
                Matcher match = Pattern.compile(patterns[p]).matcher(source);
                while (match.find()) {
                    if (code[match.start()] || code[match.end() - 1] || remove[match.start()] || escaped[match.start()] ||
                            escaped[match.end() - 1] || Character.isWhitespace(source.charAt(match.end(1) - 1))) continue;
                    range(remove, match.start(), match.start(1)); range(remove, match.end(1), match.end());
                    marks.add(new Mark(kinds[p], match.start(1), match.end(1), null));
                }
            }
            int[] positions = new int[size + 1]; StringBuilder out = new StringBuilder(size);
            for (int i = 0; i < size; ++i) { positions[i] = out.length(); if (!remove[i]) out.append(chars[i]); }
            positions[size] = out.length();
            for (Mark mark : marks) { mark.start = positions[mark.start]; mark.end = positions[mark.end]; }
            return new Result(out.toString(), positions, marks);
        }
    }

    private static final class Model {
        final String id;
        final String name;
        final List<String> efforts = new ArrayList<>();
        final String recommendedEffort;
        Model(String id, String name, JSONObject metadata) {
            this.id = id;
            this.name = name;
            // Normalized by the C++ catalog parser; includes only implemented
            // wire options that this account's model explicitly advertises.
            JSONArray options = metadata.optJSONArray("reasoning_efforts");
            if (options != null) for (int i = 0; i < options.length(); ++i) {
                String option = options.optString(i, "");
                if (!option.isEmpty() && !efforts.contains(option)) efforts.add(option);
            }
            String recommended = metadata.optString("recommended_reasoning_effort", "");
            recommendedEffort = efforts.contains(recommended) ? recommended : "";
        }
        String label() { return name; }
    }

    private static final class ChatWindow implements Application.ActivityLifecycleCallbacks, ComponentCallbacks {
        final Activity activity;
        final Application application;
        final Context ui;
        final SharedPreferences prefs;
        final Handler handler = new Handler(Looper.getMainLooper());
        final AppCompatDialog dialog;
        final LinearLayout root;
        final FrameLayout chatSurface;
        final AppCompatButton keyboardHide;
        final PopupWindow keyboardPopup;
        final LinearLayout chatPanel;
        final LinearLayout progressRow;
        final Layout settingsLayout;
        final View[] modelViews, loginViews;
        final TextView settingsStatus, settingsError;
        final AppCompatButton settings;
        final AppCompatButton activityButton;
        final AppCompatButton closeButton;
        final int backgroundColor;
        final AppCompatTextView status;
        final AppCompatTextView elapsed;
        final ProgressBar thinking;
        final AppCompatTextView error;
        final AppCompatTextView transcript;
        final TextView code;
        final TextView loginLink;
        final AppCompatEditText question;
        final Button signIn;
        final Button signOut;
        final Button newChat;
        final AppCompatButton send;
        final Button chooseModel;
        final Button chooseReasoning;
        final CheckDirectionBox internet;
        final Button savedFiles;
        final Button saveChat;
        final Button shareChat;
        final Button clearAnalysis;
        final Button stopRequest;
        final Button copyCode;
        final Button openBrowser;
        final NestedScrollView transcriptScroll;
        final List<Model> models = new ArrayList<>();
        AlertDialog childDialog;
        AppCompatDialog chartDialog;
        AppCompatTextView activityDetails;
        String activityText = "";
        boolean closed;
        boolean resumed = true;
        boolean initialized;
        boolean busy;
        boolean loggedIn;
        boolean settingsOpen;
        boolean helpOpen;
        boolean opened, settingsAttached;
        int settingsBackDepth;
        boolean haveAccountSnapshot;
        boolean modelsRequested;
        String localError = "";
        String serverError = "";
        String loginUrl = "";
        String userCode = "";
        String messageSnapshot = "";
        String modelSnapshot = "";
        String selectedModelId;
        int messageCount;
        boolean hasPendingQuestion;
        boolean forceTranscriptBottom;
        boolean restoreScroll = true;
        final Rect chatWindowBounds = new Rect();
        final Rect chatDecorBounds = new Rect();
        final Rect chatContentBounds = new Rect();
        final Rect chatAvailableBounds = new Rect();
        final Rect chatVisibleBounds = new Rect();
        final Rect chatCloseBounds = new Rect();
        final int[] chatScreenPosition = new int[2];
        String lastGeometryReport = "";
        final ViewTreeObserver.OnGlobalLayoutListener chatGeometryListener = this::applyChatGeometry;

        ChatWindow(Activity activity) {
            this.activity = activity;
            application = activity.getApplication();
            prefs = activity.getSharedPreferences("juggluco_chat", Context.MODE_PRIVATE);
            selectedModelId = prefs.getString("model", "");
            settingsOpen = running.settingsOpen;
            ui = new AlertDialog.Builder(activity).getContext();
            // Keep the Activity's non-floating window theme. A dialog-theme
            // window can still be fitted around system bars after MATCH_PARENT,
            // leaving a second navigation-bar-sized space above the keyboard.
            // Widgets retain the existing themed alert-dialog context in ui.
            dialog = new AppCompatDialog(activity, androidx.appcompat.R.style.Theme_AppCompat_Empty);
            dialog.supportRequestWindowFeature(Window.FEATURE_NO_TITLE);
            TypedArray colors = ui.obtainStyledAttributes(new int[] {android.R.attr.colorBackground});
            backgroundColor = colors.getColor(0, 0);
            colors.recycle();

            root = column();
            root.setBackgroundColor(backgroundColor);
            root.setPadding(dp(8), 0, dp(8), 0);
            root.setFocusableInTouchMode(true);
            settings = button(s(R.string.jgchat_more));
            activityButton = button(s(R.string.jgchat_activity));
            closeButton = button(s(R.string.jgchat_close));
            compactFooterButton(settings);
            compactFooterButton(activityButton);
            compactFooterButton(closeButton);

            // No title or toolbar above the editor. It grows with the draft,
            // consuming the transcript's space before it starts scrolling.
            question = new AppCompatEditText(ui);
            question.setHint(s(R.string.jgchat_ask_about_your_juggluco_data));
            question.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_MULTI_LINE
                    | InputType.TYPE_TEXT_FLAG_CAP_SENTENCES);
            question.setImeOptions(EditorInfo.IME_ACTION_NONE | EditorInfo.IME_FLAG_NO_ENTER_ACTION
                    | EditorInfo.IME_FLAG_NO_EXTRACT_UI | EditorInfo.IME_FLAG_NO_FULLSCREEN);
            question.setMinLines(1);
            question.setVerticalScrollBarEnabled(true);
            question.setGravity(Gravity.TOP | Gravity.START);
            question.setText(running.draft);
            restoreSelection();
            send = button(s(R.string.jgchat_send));
            final int editorStart = question.getPaddingStart(), editorTop = question.getPaddingTop();
            final int editorEnd = question.getPaddingEnd(), editorBottom = question.getPaddingBottom();
            FrameLayout composer = new FrameLayout(ui) {
                @Override protected void onMeasure(int width, int height) {
                    measureChild(send, width, height);
                    int end = Math.max(editorEnd, send.getMeasuredWidth() + dp(8));
                    if (question.getPaddingEnd() != end)
                        question.setPaddingRelative(editorStart, editorTop, end, editorBottom);
                    super.onMeasure(width, height);
                }
            };
            composer.addView(question, new FrameLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
            composer.addView(send, new FrameLayout.LayoutParams(
                    ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT, Gravity.TOP | Gravity.END));
            chatPanel = new LinearLayout(ui) {
                @Override protected void onMeasure(int width, int height) {
                    if (View.MeasureSpec.getMode(height) != View.MeasureSpec.UNSPECIFIED) {
                        int available = Math.max(1, View.MeasureSpec.getSize(height) - getPaddingTop() - getPaddingBottom());
                        if (question.getMaxHeight() != available) question.setMaxHeight(available);
                    }
                    super.onMeasure(width, height);
                }
            };
            chatPanel.setOrientation(LinearLayout.VERTICAL);
            chatPanel.addView(composer, fullWidth());
            transcript = text("");
            transcript.setTextIsSelectable(true);
            transcript.setMovementMethod(LinkMovementMethod.getInstance());
            transcript.setPadding(dp(2), dp(4), dp(2), 0);
            transcriptScroll = new NestedScrollView(ui);
            transcriptScroll.setFillViewport(true);
            transcriptScroll.addView(transcript, new ViewGroup.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
            chatPanel.addView(transcriptScroll, new LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, 0, 1));
            root.addView(chatPanel, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1));

            // Use the Activity context and Juggluco's normal widgets/theme.
            TextView settingsTitle = util.getlabel(activity, R.string.jgchat_settings);
            signIn = util.getbutton(activity, R.string.jgchat_sign_in);
            signOut = util.getbutton(activity, R.string.jgchat_sign_out);
            newChat = util.getbutton(activity, R.string.jgchat_new_chat);
            savedFiles = util.getbutton(activity, R.string.jgchat_saved_files);
            saveChat = util.getbutton(activity, R.string.jgchat_save_chat_a8796cea);
            shareChat = util.getbutton(activity, R.string.jgchat_share_chat);
            clearAnalysis = util.getbutton(activity, R.string.jgchat_clear_analysis_memory);
            TextView modelLabel = util.getlabel(activity, R.string.jgchat_model);
            chooseModel = util.getbutton(activity, R.string.jgchat_select_a_model);
            chooseReasoning = util.getbutton(activity, R.string.jgchat_reasoning_server_default);
            internet = util.getcheckbox(activity, R.string.jgchat_internet_search, prefs.getBoolean("internet_search", true));
            internet.setOnCheckedChangeListener((button, checked) -> prefs.edit().putBoolean("internet_search", checked).apply());
            modelViews = new View[]{modelLabel, chooseModel, chooseReasoning, internet};
            TextView loginCodeLabel = util.getlabel(activity, R.string.jgchat_sign_in_code);
            code = util.getlabel(activity, "");
            code.setTextSize(22);
            code.setTextIsSelectable(true);
            loginLink = util.getlabel(activity, "");
            loginLink.setTextIsSelectable(true);
            loginLink.setMaxLines(2);
            loginLink.setEllipsize(TextUtils.TruncateAt.END);
            copyCode = util.getbutton(activity, R.string.jgchat_copy_code);
            openBrowser = util.getbutton(activity, R.string.jgchat_open_sign_in_page);
            loginViews = new View[]{loginCodeLabel, code, loginLink, copyCode, openBrowser};
            stopRequest = util.getbutton(activity, R.string.jgchat_stop_request);
            Button helpbutton = util.getbutton(activity, R.string.helpname);
            Button settingsActivity = util.getbutton(activity, R.string.jgchat_activity);
            Button settingsClose = util.getbutton(activity, R.string.jgchat_close);
            settingsStatus = util.getlabel(activity, "");
            settingsStatus.setMaxLines(1);
            settingsStatus.setEllipsize(TextUtils.TruncateAt.END);
            settingsError = util.getlabel(activity, "");
            settingsError.setMaxLines(3);
            settingsError.setTextIsSelectable(true);
            settingsError.setVisibility(View.GONE);

            settingsLayout = new Layout(activity,
                    new View[]{settingsTitle, signIn, signOut},
                    new View[]{modelLabel, chooseModel, chooseReasoning, internet},
                    new View[]{newChat, savedFiles, saveChat, shareChat},
                    new View[]{clearAnalysis, stopRequest},
                    new View[]{loginCodeLabel, code, copyCode, openBrowser},
                    new View[]{loginLink},
                    new View[]{settingsStatus, settingsError},
                    new View[]{helpbutton, settingsActivity, settingsClose}
            ).portraitLayout(
                    new View[]{settingsTitle},
                    new View[]{modelLabel, chooseModel},
                    new View[]{chooseReasoning},
                    new View[]{internet},
                    new View[]{saveChat, shareChat},
                    new View[]{newChat, savedFiles},
                    new View[]{clearAnalysis},
                    new View[]{stopRequest},
                    new View[]{signIn, signOut},
                    new View[]{loginCodeLabel, code},
                    new View[]{copyCode, openBrowser},
                    new View[]{loginLink},
                    new View[]{settingsStatus},
                    new View[]{settingsError},
                    new View[]{helpbutton, settingsActivity, settingsClose});
            settingsLayout.setBackgroundColor(Applic.backgroundcolor);
            settingsLayout.systembarPadding((left, top, right, bottom) -> new int[]{left + dp(8), top, right + dp(8), bottom});
            helpbutton.setOnClickListener(v -> showHelp());
            settingsActivity.setOnClickListener(v -> showActivity());
            settingsClose.setOnClickListener(v -> MainActivity.doonback());

            status = text(s(R.string.jgchat_opening_chat));
            status.setMaxLines(1);
            status.setEllipsize(TextUtils.TruncateAt.END);
            thinking = new ProgressBar(ui);
            thinking.setIndeterminate(true);
            elapsed = text("");
            elapsed.setPadding(dp(8), 0, 0, 0);
            LinearLayout progressButtons = new LinearLayout(ui);
            progressButtons.setGravity(Gravity.CENTER_VERTICAL);
            progressButtons.addView(thinking, new LinearLayout.LayoutParams(dp(24), dp(24)));
            progressButtons.addView(status, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));
            progressButtons.addView(elapsed);
            progressRow = column();
            progressRow.addView(progressButtons, fullWidth());
            progressRow.setOnClickListener(v -> showActivity());
            progressRow.setContentDescription(s(R.string.jgchat_thinking_progress_tap_for_activity_details));
            root.addView(progressRow, fullWidth());
            error = text("");
            error.setTypeface(error.getTypeface(), Typeface.BOLD);
            error.setTextIsSelectable(true);
            error.setMaxLines(3);
            error.setVerticalScrollBarEnabled(true);
            error.setVisibility(View.GONE);
            root.addView(error, fullWidth());
            root.addView(row(settings, activityButton, closeButton), fullWidth());

            // A separate non-focusable popup can sit over the keyboard's top
            // row in portrait, without taking focus from the editor or
            // reserving space in the transcript.
            chatSurface = new FrameLayout(ui);
            chatSurface.addView(root, new FrameLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
            keyboardHide = button(s(R.string.jgchat_hide));
            keyboardHide.setContentDescription(s(R.string.jgchat_hide_keyboard));
            keyboardHide.setMinHeight(dp(40));
            keyboardHide.setMinimumHeight(0);
            keyboardHide.setPadding(dp(16), dp(4), dp(16), dp(4));
            keyboardHide.setIncludeFontPadding(false);
            keyboardHide.setFocusable(false);
            GradientDrawable hideBackground = new GradientDrawable();
            hideBackground.setColor(backgroundColor);
            hideBackground.setCornerRadius(dp(20));
            hideBackground.setStroke(dp(1), keyboardHide.getCurrentTextColor());
            keyboardHide.setBackground(hideBackground);
            keyboardHide.setElevation(dp(8));
            keyboardHide.setOnClickListener(v -> { hideKeyboard(); root.requestFocus(); });
            keyboardPopup = new PopupWindow(keyboardHide,
                    ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT, false);
            keyboardPopup.setInputMethodMode(PopupWindow.INPUT_METHOD_NOT_NEEDED);
            keyboardPopup.setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_NOTHING);
            keyboardPopup.setOutsideTouchable(false);
            keyboardPopup.setClippingEnabled(false);
            keyboardPopup.setAnimationStyle(0);
            if (Build.VERSION.SDK_INT >= 22) keyboardPopup.setAttachedInDecor(true);
            if (Build.VERSION.SDK_INT >= 29) {
                // Keep the popup relative to the dialog. LAYOUT_IN_SCREEN
                // can use a separate navigation-bar-inset frame, shifting
                // screen-coordinate x positions off the right edge when
                // navigation buttons are on the left.
                keyboardPopup.setIsLaidOutInScreen(false);
                keyboardPopup.setTouchModal(false);
            }
            dialog.setContentView(chatSurface, new ViewGroup.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
            dialog.setCanceledOnTouchOutside(false);
            dialog.setOnDismissListener(ignored -> close());
            dialog.getOnBackPressedDispatcher().addCallback(dialog, new OnBackPressedCallback(true) {
                @Override public void handleOnBackPressed() {
                    dialogClose();
                }
            });
            configureWindow();
            root.requestFocus();
            settings.setOnClickListener(v -> showSettings(true));
            activityButton.setOnClickListener(v -> showActivity());
            closeButton.setOnClickListener(v -> dialogClose());

            signIn.setOnClickListener(v -> {
                hideKeyboard();
                modelsRequested = false;
                invoke(JugglucoChat::nativeStartLoginMessages);
            });
            signOut.setOnClickListener(v -> {
                modelsRequested = false;
                if (invoke(JugglucoChat::nativeLogoutMessages)) question.setText("");
            });
            newChat.setOnClickListener(v -> confirmNewChat());
            chooseModel.setOnClickListener(v -> showModels());
            chooseReasoning.setOnClickListener(v -> showReasoning());
            savedFiles.setOnClickListener(v -> showSavedFiles());
            saveChat.setOnClickListener(v -> chooseChatExport(false));
            shareChat.setOnClickListener(v -> chooseChatExport(true));
            clearAnalysis.setOnClickListener(v -> confirmClearAnalysis());
            stopRequest.setOnClickListener(v -> { running.releaseWake(); invoke(JugglucoChat::nativeCancelMessages); });
            send.setOnClickListener(v -> sendQuestion());
            openBrowser.setOnClickListener(v -> openLogin());
            copyCode.setOnClickListener(v -> copyLoginCode());
            TextWatcher controls = new TextWatcher() {
                @Override public void beforeTextChanged(CharSequence s, int start, int count, int after) {}
                @Override public void onTextChanged(CharSequence s, int start, int before, int count) {}
                @Override public void afterTextChanged(Editable s) { saveDraft(); updateControls(); }
            };
            question.addTextChangedListener(controls);
            updateControls();
        }

        String s(int id, Object... args) { return ui.getString(id, args); }
        int dp(int value) { return Math.round(value * activity.getResources().getDisplayMetrics().density); }
        LinearLayout.LayoutParams fullWidth() {
            return new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        }
        LinearLayout column() {
            LinearLayout out = new LinearLayout(ui);
            out.setOrientation(LinearLayout.VERTICAL);
            return out;
        }
        AppCompatTextView text(String value) {
            AppCompatTextView out = new AppCompatTextView(ui);
            out.setText(value);
            return out;
        }
        AppCompatButton button(String value) {
            AppCompatButton out = new AppCompatButton(ui);
            out.setText(value);
            out.setAllCaps(false);
            out.setMinWidth(0);
            out.setMinimumWidth(0);
            return out;
        }
        void compactFooterButton(AppCompatButton button) {
            // The theme's 48 dp minimum left a large blank band above and
            // below the labels. Keep WRAP_CONTENT for larger font settings.
            button.setMinHeight(dp(32));
            button.setMinimumHeight(0);
            button.setIncludeFontPadding(false);
            button.setPaddingRelative(button.getPaddingStart(), dp(2), button.getPaddingEnd(), dp(2));
        }
        LinearLayout row(View... children) {
            LinearLayout out = new LinearLayout(ui);
            out.setGravity(Gravity.CENTER_VERTICAL);
            for (View child : children) out.addView(child,
                    new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));
            return out;
        }

        void open() {
            opened = true;
            application.registerActivityLifecycleCallbacks(this);
            application.registerComponentCallbacks(this);
            try {
                NativeMessage result = nativeInitWithFilesMessages(activity.getNoBackupFilesDir().getAbsolutePath() + "/juggluco-chat",
                        activity.getFilesDir().getAbsolutePath());
                initialized = result == null;
                if (!initialized) localError = result.text(ui);
            } catch (RuntimeException | LinkageError failure) {
                localError = message(failure);
            }
            if (initialized) {
                running.initialized = true;
                refresh();
                if (running.restoreKeyboard && loggedIn && !settingsOpen) {
                    running.restoreKeyboard = false;
                    question.requestFocus();
                    handler.post(() -> {
                        if (!closed && dialog.getWindow() != null)
                            WindowCompat.getInsetsController(dialog.getWindow(), question).show(WindowInsetsCompat.Type.ime());
                    });
                }
            }
            else {
                status.setText(s(R.string.jgchat_chat_could_not_be_opened));
                showError();
            }
            updateControls();
        }

        void refresh() {
            if (!closed && initialized) running.refresh();
        }
        void saveDraft() {
            running.draft = question.getText().toString();
            saveSelection();
        }
        void saveSelection() {
            // A hidden poll may have restored a failed question to the draft
            // before this old window is torn down. Do not overwrite that text.
            if (!TextUtils.equals(question.getText(), running.draft)) return;
            running.selectionStart = Math.max(0, question.getSelectionStart());
            running.selectionEnd = Math.max(0, question.getSelectionEnd());
        }
        void restoreSelection() {
            int length = question.length();
            question.setSelection(Math.min(length, running.selectionStart), Math.min(length, running.selectionEnd));
        }
        void saveScroll() {
            if (restoreScroll) return;
            running.scrollY = transcriptScroll.getScrollY();
            running.followTail = transcript.getHeight() - (running.scrollY + transcriptScroll.getHeight()) <= dp(48);
        }

        void apply(JSONObject snapshot) {
            busy = snapshot.optBoolean("busy", false);
            boolean wasLoggedIn = loggedIn;
            loggedIn = snapshot.optBoolean("logged_in", false);
            if (haveAccountSnapshot && loggedIn != wasLoggedIn) {
                settingsOpen = false;
                running.settingsOpen = false;
                hideKeyboard();
                root.requestFocus();
            }
            haveAccountSnapshot = true;
            serverError = NativeMessage.text(ui, snapshot.optJSONObject("error"));
            String description = NativeMessage.text(ui, snapshot.optJSONObject("status"));
            status.setText(description.isEmpty() ? (loggedIn ? s(R.string.jgchat_signed_in) : s(R.string.jgchat_sign_in_to_begin)) : description);
            elapsed.setText(s(R.string.jgchat_elapsed, duration(snapshot.optLong("elapsed_ms", 0))));
            StringBuilder activityLines = new StringBuilder();
            JSONArray events = snapshot.optJSONArray("activity");
            if (events != null) for (int i = 0; i < events.length(); ++i) {
                JSONObject entry = events.optJSONObject(i);
                if (entry != null) activityLines.append(duration(entry.optLong("elapsed_ms", 0)))
                        .append("  ").append(NativeMessage.text(ui, entry.optJSONObject("message"))).append("\n\n");
            }
            activityText = activityLines.length() == 0 ? description : activityLines.toString();
            if (activityDetails != null) activityDetails.setText(activityText);
            loginUrl = snapshot.optString("login_url", "");
            userCode = snapshot.optString("user_code", "");
            code.setText(userCode);
            loginLink.setText(loginUrl);
            JSONArray available = snapshot.optJSONArray("models");
            updateModels(available == null ? new JSONArray() : available);
            JSONArray messages = snapshot.optJSONArray("messages");
            if (messages == null) messages = new JSONArray();
            messageCount = messages.length();
            hasPendingQuestion = !snapshot.optString("pending_question", "").isEmpty();
            renderMessages(messages, snapshot.optString("pending_question", ""));
            if (!TextUtils.equals(question.getText(), running.draft)) {
                int start = running.selectionStart, end = running.selectionEnd;
                question.setText(running.draft);
                running.selectionStart = start; running.selectionEnd = end;
                restoreSelection();
            }
            showError();
            updateControls();
            if (resumed && !busy) {
                try {
                    String pending = nativeTakeBrowserFile();
                    if (pending != null) {
                        JSONObject file = new JSONObject(pending);
                        openSavedFile(file.getString("id"), file.getString("file"));
                    }
                } catch (JSONException | RuntimeException | LinkageError failure) {
                    localError = s(R.string.jgchat_saved_page_error, message(failure)); showError();
                }
            }
            if (resumed && loggedIn && !busy && models.isEmpty() && !modelsRequested) {
                modelsRequested = true;
                invoke(JugglucoChat::nativeLoadModelsMessages);
            }
        }

        void updateModels(JSONArray available) {
            String encoded = available.toString();
            if (encoded.equals(modelSnapshot)) return;
            modelSnapshot = encoded;
            models.clear();
            ArrayList<String> ids = new ArrayList<>();
            ArrayList<String> names = new ArrayList<>();
            for (int i = 0; i < available.length(); ++i) {
                JSONObject item = available.optJSONObject(i);
                if (item == null) continue;
                String id = item.optString("id", "").trim();
                if (id.isEmpty() || ids.contains(id)) continue;
                String name = item.optString("display_name", "").trim();
                if (name.isEmpty() || !Character.isUpperCase(name.codePointAt(0)) || !name.contains("GPT")) continue;
                String folded = name.toLowerCase(Locale.ROOT);
                if (names.contains(folded)) continue;
                models.add(new Model(id, name, item));
                names.add(folded);
                ids.add(id);
            }
            if (!ids.isEmpty()) {
                modelsRequested = true;
                if (!ids.contains(selectedModelId)) selectedModelId = ids.get(0);
                prefs.edit().putString("model", selectedModelId).apply();
            }
        }

        void renderMessages(JSONArray messages, String pending) {
            String encoded = messages.toString() + "\n" + pending + (busy ? "\nbusy" : "\nidle");
            if (encoded.equals(messageSnapshot)) return;
            messageSnapshot = encoded;
            boolean bottom = forceTranscriptBottom || (restoreScroll ? running.followTail : transcript.getHeight()
                    - (transcriptScroll.getScrollY() + transcriptScroll.getHeight()) <= dp(48));
            final int restoreY = restoreScroll ? running.scrollY : transcriptScroll.getScrollY();
            SpannableStringBuilder text = new SpannableStringBuilder();
            for (int i = 0; i < messages.length(); ++i) {
                JSONObject item = messages.optJSONObject(i);
                if (item == null) continue;
                String role = item.optString("role");
                if (!"user".equals(role) && !"assistant".equals(role)) continue;
                if (text.length() > 0) text.append("\n\n");
                int start = text.length();
                String heading = "user".equals(role) ? s(R.string.jgchat_you) : answerHeading(
                        item.optString("model_name", ""), item.optString("model_id", ""), s(R.string.jgchat_chatgpt));
                if ("assistant".equals(role)) {
                    int level = reasoningAbbreviation(item.has("reasoning_effort") && !item.isNull("reasoning_effort")
                            ? item.optString("reasoning_effort", null) : null);
                    if (level != 0) heading = s(R.string.jgchat_answer_heading, heading, s(level));
                }
                text.append(heading);
                text.setSpan(new StyleSpan(Typeface.BOLD), start, text.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                text.append("\n");
                final int bodyStart = text.length();
                final String body = item.optString("text", "");
                final Markup.Result formatted = "assistant".equals(role) ? Markup.parse(body) : null;
                text.append(formatted == null ? body : formatted.text);
                if ("assistant".equals(role)) {
                    applyMarkup(text, bodyStart, formatted);
                    linkWebText(text, bodyStart, formatted.text);
                    JSONArray citations = item.optJSONArray("citations");
                    if (citations != null) for (int c = 0; c < Math.min(64, citations.length()); ++c) {
                        JSONObject source = citations.optJSONObject(c);
                        if (source == null) continue;
                        String url = source.optString("url", "");
                        if (!webUrl(url)) continue;
                        int first = source.optInt("start", -1), last = source.optInt("end", -1);
                        int count = body.codePointCount(0, body.length());
                        if (first >= 0 && last > first && last <= count)
                            link(text, bodyStart + formatted.positions[body.offsetByCodePoints(0, first)],
                                    bodyStart + formatted.positions[body.offsetByCodePoints(0, last)], () -> openWebUrl(url));
                        text.append("\n");
                        int sourceStart = text.length();
                        text.append("[").append(Integer.toString(c + 1)).append("] ").append(source.optString("title", url));
                        link(text, sourceStart, text.length(), () -> openWebUrl(url));
                    }
                }
                JSONArray plots = item.optJSONArray("plots");
                if (plots != null) for (int p = 0; p < Math.min(4, plots.length()); ++p) {
                    JSONObject plot = plots.optJSONObject(p);
                    if (plot == null) continue;
                    final int messageIndex = i, plotIndex = p;
                    text.append("\n\n");
                    int linkStart = text.length();
                    text.append(s(R.string.jgchat_open_plot_value, plot.optString("caption", s(R.string.jgchat_plot))));
                    text.setSpan(new ClickableSpan() {
                        @Override public void onClick(View view) { showPlot(messageIndex, plotIndex); }
                        @Override public void updateDrawState(TextPaint paint) {
                            paint.setColor(transcript.getCurrentTextColor());
                            paint.setUnderlineText(true);
                        }
                    }, linkStart, text.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                }
                JSONArray files = item.optJSONArray("files");
                if (files != null) for (int f = 0; f < Math.min(4, files.length()); ++f) {
                    JSONObject bundle = files.optJSONObject(f);
                    if (bundle == null) continue;
                    String id = bundle.optString("id", ""), entry = bundle.optString("entrypoint", "jg-files.html");
                    text.append("\n\n");
                    int fileStart = text.length();
                    text.append(s(R.string.jgchat_open_in_chrome_value, bundle.optString("title", s(R.string.jgchat_saved_files))));
                    link(text, fileStart, text.length(), () -> openSavedFile(id, entry));
                    text.append("\n"); fileStart = text.length();
                    text.append(s(R.string.jgchat_view_download_files));
                    link(text, fileStart, text.length(), () -> openSavedFile(id, "jg-files.html"));
                    text.append("\n"); fileStart = text.length();
                    text.append(s(R.string.jgchat_save_as));
                    link(text, fileStart, text.length(), () -> showExportFiles(bundle));
                }
            }
            if (!pending.isEmpty()) {
                if (text.length() > 0) text.append("\n\n");
                int start = text.length();
                text.append(s(R.string.jgchat_you));
                text.setSpan(new StyleSpan(Typeface.BOLD), start, text.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                text.append("\n").append(pending).append("\n\n");
                text.append(busy ? s(R.string.jgchat_answer_in_progress) : s(R.string.jgchat_no_answer_was_completed_send_the_question_again_to_retry));
            }
            if (text.length() == 0) text.append(s(R.string.jgchat_your_conversation_will_appear_here));
            transcript.setText(text);
            handler.post(() -> {
                if (!closed) {
                    if (bottom) transcriptScroll.fullScroll(View.FOCUS_DOWN);
                    else transcriptScroll.scrollTo(0, restoreY);
                }
            });
            restoreScroll = false;
            forceTranscriptBottom = false;
        }

        void applyMarkup(SpannableStringBuilder text, int offset, Markup.Result result) {
            for (Markup.Mark mark : result.marks) {
                if (mark.end <= mark.start) continue;
                int start = offset + mark.start, end = offset + mark.end;
                if (mark.kind == Markup.LINK) {
                    if (webUrl(mark.url)) link(text, start, end, () -> openWebUrl(mark.url));
                    continue;
                }
                Object span = mark.kind == Markup.CODE ? new TypefaceSpan("monospace") :
                        mark.kind == Markup.QUOTE ? new QuoteSpan(transcript.getCurrentTextColor()) :
                        new StyleSpan(mark.kind == Markup.ITALIC ? Typeface.ITALIC :
                                mark.kind == Markup.BOTH ? Typeface.BOLD_ITALIC : Typeface.BOLD);
                text.setSpan(span, start, end, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                if (mark.kind == Markup.HEADING)
                    text.setSpan(new RelativeSizeSpan(1.12f), start, end, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
            }
        }

        void link(SpannableStringBuilder text, int start, int end, Runnable action) {
            if (end <= start) return;
            text.setSpan(new ClickableSpan() {
                @Override public void onClick(View view) { action.run(); }
                @Override public void updateDrawState(TextPaint paint) {
                    paint.setColor(transcript.getCurrentTextColor()); paint.setUnderlineText(true);
                }
            }, start, end, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        }
        boolean webUrl(String url) {
            Uri uri = Uri.parse(url);
            return ("https".equals(uri.getScheme()) || "http".equals(uri.getScheme()))
                    && uri.getHost() != null && !uri.getHost().isEmpty();
        }
        void linkWebText(SpannableStringBuilder text, int offset, String body) {
            Matcher found = Pattern.compile("https?://[^\\s<>\\\"\\]\\)]+").matcher(body);
            while (found.find()) {
                int end = found.end();
                while (end > found.start() && ".,;".indexOf(body.charAt(end - 1)) >= 0) --end;
                final String url = body.substring(found.start(), end);
                if (webUrl(url)) link(text, offset + found.start(), offset + end, () -> openWebUrl(url));
            }
        }
        void openWebUrl(String url) {
            if (!webUrl(url)) return;
            hideKeyboard();
            try {
                Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(url)).addCategory(Intent.CATEGORY_BROWSABLE);
                try { activity.startActivity(new Intent(intent).setPackage("com.android.chrome")); }
                catch (ActivityNotFoundException unavailable) { activity.startActivity(intent); }
            } catch (RuntimeException failure) {
                // Do not include the URL: local URLs can contain an API secret.
                localError = s(R.string.jgchat_could_not_open_a_browser); showError();
            }
        }
        void openSavedFile(String id, String name) {
            try {
                JSONObject result = new JSONObject(nativeFileUrlMessages(id, name));
                if (result.has("error")) { localError = NativeMessage.text(ui, result.optJSONObject("error")); showError(); return; }
                String url = result.getString("url");
                if (!"127.0.0.1".equals(Uri.parse(url).getHost())) throw new JSONException(s(R.string.jgchat_invalid_local_file_url));
                openWebUrl(url);
            } catch (JSONException | RuntimeException | LinkageError failure) {
                localError = s(R.string.jgchat_could_not_open_the_saved_file); showError();
            }
        }
        void showSavedFiles() {
            dismissChild();
            try {
                JSONObject result = new JSONObject(nativeListFilesMessages());
                if (result.has("error")) { localError = NativeMessage.text(ui, result.optJSONObject("error")); showError(); return; }
                JSONArray files = result.getJSONArray("files");
                if (files.length() == 0) {
                    childDialog = new AlertDialog.Builder(ui).setTitle(s(R.string.jgchat_saved_files))
                            .setMessage(s(R.string.jgchat_ask_for_an_html_javascript_page_csv_json_or_text_file_saved_files_remain_available_aft))
                            .setPositiveButton(s(R.string.jgchat_close), null).create();
                } else {
                    String[] labels = new String[files.length()];
                    for (int i = 0; i < labels.length; ++i) {
                        JSONObject bundle = files.getJSONObject(i);
                        String id = bundle.optString("id", "");
                        labels[i] = bundle.optString("title", s(R.string.jgchat_files)) + " · " + id.substring(0, Math.min(6, id.length()));
                    }
                    childDialog = new AlertDialog.Builder(ui).setTitle(s(R.string.jgchat_saved_files))
                            .setItems(labels, (dialog, which) -> {
                                JSONObject bundle = files.optJSONObject(which);
                                if (bundle != null) showFileActions(bundle);
                            }).setNegativeButton(s(R.string.jgchat_close), null).create();
                }
                childDialog.show();
            } catch (JSONException | RuntimeException | LinkageError failure) {
                localError = s(R.string.jgchat_could_not_list_saved_files); showError();
            }
        }
        void showFileActions(JSONObject bundle) {
            dismissChild();
            final String id = bundle.optString("id", "");
            childDialog = new AlertDialog.Builder(ui).setTitle(bundle.optString("title", s(R.string.jgchat_files)))
                    .setMessage(s(R.string.jgchat_save_as_lets_you_choose_a_file_and_its_destination_in_android_open_uses_juggluco_s_web))
                    .setPositiveButton(s(R.string.jgchat_open), (d, w) -> openSavedFile(id, bundle.optString("entrypoint", "jg-files.html")))
                    .setNeutralButton(s(R.string.jgchat_save_as), (d, w) -> showExportFiles(bundle))
                    .setNegativeButton(s(R.string.jgchat_close), null).create();
            childDialog.show();
        }

        void showExportFiles(JSONObject bundle) {
            dismissChild();
            final ArrayList<String> names = new ArrayList<>();
            JSONArray files = bundle.optJSONArray("files");
            boolean hasHtml = false;
            if (files != null) for (int i = 0; i < Math.min(8, files.length()); ++i) {
                JSONObject item = files.optJSONObject(i);
                String name = item == null ? "" : item.optString("name", "");
                if (!name.isEmpty()) {
                    names.add(name);
                    hasHtml |= name.endsWith(".html") || name.endsWith(".htm");
                }
            }
            if (hasHtml) names.add("jg-api.js");
            if (names.isEmpty()) { localError = s(R.string.jgchat_no_generated_files_are_available); showError(); return; }
            final String id = bundle.optString("id", "");
            if (names.size() == 1) { saveDocument(id, names.get(0)); return; }
            childDialog = new AlertDialog.Builder(ui).setTitle(s(R.string.jgchat_save_a_file_as))
                    .setItems(names.toArray(new String[0]), (ignored, which) -> saveDocument(id, names.get(which)))
                    .setNegativeButton(s(R.string.jgchat_cancel), null).create();
            childDialog.show();
        }
        @SuppressWarnings("deprecation")
        void saveDocument(String id, String name) {
            if (closed || !resumed) return;
            hideKeyboard();
            try {
                FragmentManager manager = activity.getFragmentManager();
                if (manager.findFragmentByTag(SaveFileFragment.TAG) != null) {
                    localError = s(R.string.jgchat_file_save_in_progress); showError(); return;
                }
                SaveFileFragment fragment = new SaveFileFragment();
                Bundle args = new Bundle(); args.putString("id", id); args.putString("file", name);
                fragment.setArguments(args);
                manager.beginTransaction().add(fragment, SaveFileFragment.TAG).commit();
                manager.executePendingTransactions();
                localError = ""; showError();
            } catch (RuntimeException failure) {
                localError = s(R.string.jgchat_could_not_start_save_as_return_to_the_chat_and_try_again); showError();
            }
        }

        void updateControls() {
            if (closed) return;
            boolean loginActive = busy && (!loginUrl.isEmpty() || !userCode.isEmpty());
            boolean accountPage = !loggedIn || settingsOpen || loginActive;
            for (View view : modelViews) view.setVisibility(loggedIn ? View.VISIBLE : View.GONE);
            for (View view : loginViews) view.setVisibility(loginActive ? View.VISIBLE : View.GONE);
            settings.setVisibility(loggedIn && !accountPage ? View.VISIBLE : View.GONE);
            settings.setEnabled(initialized);
            activityButton.setVisibility(loggedIn || busy ? View.VISIBLE : View.GONE);
            activityButton.setEnabled(initialized);
            closeButton.setText(s(R.string.jgchat_close));
            progressRow.setVisibility(busy || !loggedIn ? View.VISIBLE : View.GONE);
            thinking.setVisibility(busy ? View.VISIBLE : View.GONE);
            elapsed.setVisibility(busy ? View.VISIBLE : View.GONE);
            newChat.setVisibility(loggedIn ? View.VISIBLE : View.GONE);
            signIn.setText(loggedIn ? s(R.string.jgchat_sign_in_again) : s(R.string.jgchat_sign_in));
            signIn.setEnabled(initialized && !busy);
            signOut.setEnabled(initialized && !busy);
            newChat.setEnabled(initialized && !busy);
            chooseModel.setEnabled(initialized && loggedIn && !busy);
            chooseReasoning.setEnabled(initialized && loggedIn && !busy);
            internet.setEnabled(initialized && loggedIn && !busy);
            savedFiles.setEnabled(initialized && !busy);
            boolean hasChat = messageCount > 0 || hasPendingQuestion;
            saveChat.setEnabled(initialized && hasChat);
            shareChat.setEnabled(initialized && hasChat);
            clearAnalysis.setVisibility(loggedIn ? View.VISIBLE : View.GONE);
            clearAnalysis.setEnabled(initialized && !busy);
            stopRequest.setVisibility(busy ? View.VISIBLE : View.GONE);
            stopRequest.setEnabled(initialized && busy);
            Model selected = selectedModel();
            chooseModel.setText(selected == null ? s(R.string.jgchat_select_a_model) : selected.name.isEmpty() ? selected.id : selected.name);
            chooseReasoning.setText(s(R.string.jgchat_reasoning_value, effortLabel(selectedEffort())));
            question.setEnabled(initialized && loggedIn);
            String hint = selected == null
                    ? s(R.string.jgchat_choose_a_model_in_settings_then_ask_a_question) : s(R.string.jgchat_ask_about_your_juggluco_data);
            if (!TextUtils.equals(question.getHint(), hint)) question.setHint(hint);
            send.setEnabled(initialized && loggedIn && !busy
                    && selected != null
                    && !question.getText().toString().trim().isEmpty());
            openBrowser.setEnabled(busy && allowedLoginUrl(loginUrl));
            copyCode.setEnabled(busy && !userCode.isEmpty());
            settingsStatus.setText(status.getText());
            settingsStatus.setVisibility(busy || !loggedIn ? View.VISIBLE : View.GONE);
            displayPage(accountPage);
        }
        void chooseChatExport(boolean share) {
            dismissChild();
            childDialog = new AlertDialog.Builder(ui).setTitle(share ? s(R.string.jgchat_share_chat) : s(R.string.jgchat_save_chat_a8796cea))
                    .setItems(new String[]{s(R.string.jgchat_html_conversation_and_plots), s(R.string.jgchat_text_conversation_without_images)},
                            (ignored, which) -> exportDocument(share, which == 0 ? "html" : "txt", null))
                    .setNegativeButton(s(R.string.jgchat_cancel), null).create();
            childDialog.show();
        }
        @SuppressWarnings("deprecation")
        void exportDocument(boolean share, String format, String svg) {
            if (closed || !resumed) return;
            hideKeyboard();
            try {
                FragmentManager manager = activity.getFragmentManager();
                if (manager.findFragmentByTag(SaveFileFragment.TAG) != null) {
                    localError = s(R.string.jgchat_file_export_in_progress); showError(); return;
                }
                Bundle args = new Bundle(); args.putString("format", format); args.putBoolean("share", share);
                if (svg != null) args.putString("svg", svg);
                SaveFileFragment fragment = new SaveFileFragment(); fragment.setArguments(args);
                manager.beginTransaction().add(fragment, SaveFileFragment.TAG).commit();
                manager.executePendingTransactions();
                localError = ""; showError();
            } catch (RuntimeException failure) {
                localError = s(R.string.jgchat_could_not_start_the_export); showError();
            }
        }
        void showError() {
            String value = localError.isEmpty() ? serverError : localError;
            error.setText(value.isEmpty() ? "" : s(R.string.jgchat_error_value, value));
            error.setVisibility(value.isEmpty() ? View.GONE : View.VISIBLE);
            settingsError.setText(error.getText());
            settingsError.setVisibility(error.getVisibility());
        }
        String message(Throwable failure) {
            String value = failure.getMessage();
            return value == null || value.isEmpty() ? failure.getClass().getSimpleName() : value;
        }

        boolean invoke(NativeCall call) {
            if (closed || !initialized) return false;
            try {
                NativeMessage result = call.run();
                localError = result == null ? "" : result.text(ui);
                if (result != null) { showError(); return false; }
                refresh();
                return true;
            } catch (RuntimeException | LinkageError failure) {
                localError = message(failure);
                showError();
                return false;
            }
        }
        void sendQuestion() {
            String draft = question.getText().toString().trim();
            Model selected = selectedModel();
            String effort = selectedEffort();
            if (closed || !initialized || draft.isEmpty() || selected == null || busy || !loggedIn) return;
            try {
                running.publishPhoneActivity();
                NativeMessage result = nativeSendWithOptionsMessages(draft, selected.id, effort, internet.isChecked());
                localError = result == null ? "" : result.text(ui);
                if (result != null) { showError(); return; }
                // The native snapshot owns the submitted question, including
                // while this view is absent. The editor is the next draft.
                question.setText("");
                forceTranscriptBottom = true;
                hideKeyboard(); root.requestFocus();
                refresh();
            } catch (RuntimeException | LinkageError failure) {
                localError = message(failure); showError();
            }
        }

        void confirmNewChat() {
            if (messageCount == 0) { startNewChat(); return; }
            dismissChild();
            childDialog = new AlertDialog.Builder(ui)
                    .setTitle(s(R.string.jgchat_new_chat))
                    .setMessage(s(R.string.jgchat_start_a_new_conversation_saved_results_working_notes_and_earlier_questions_remain_avai))
                    .setPositiveButton(s(R.string.jgchat_new_chat), (ignored, which) -> startNewChat())
                    .setNegativeButton(s(R.string.jgchat_cancel), null).create();
            childDialog.show();
        }
        void startNewChat() {
            if (invoke(JugglucoChat::nativeNewChatMessages)) {
                question.setText("");
                showSettings(false);
            }
        }
        void confirmClearAnalysis() {
            dismissChild();
            childDialog = new AlertDialog.Builder(ui)
                    .setTitle(s(R.string.jgchat_clear_analysis_memory))
                    .setMessage(s(R.string.jgchat_delete_saved_analysis_results_notes_and_archived_conversations_and_start_a_new_chat_ju))
                    .setPositiveButton(s(R.string.jgchat_clear), (ignored, which) -> {
                        if (invoke(JugglucoChat::nativeClearAnalysisMessages)) { question.setText(""); showSettings(false); }
                    }).setNegativeButton(s(R.string.jgchat_cancel), null).create();
            childDialog.show();
        }
        void showModels() {
            dismissChild();
            AlertDialog.Builder builder = new AlertDialog.Builder(ui).setTitle(s(R.string.jgchat_models))
                    .setNegativeButton(s(R.string.jgchat_close), null)
                    .setNeutralButton(s(R.string.jgchat_reload), (ignored, which) -> {
                        modelsRequested = true;
                        invoke(JugglucoChat::nativeLoadModelsMessages);
                    });
            if (models.isEmpty()) {
                builder.setMessage(s(R.string.jgchat_no_capitalized_gpt_model_names_are_available_reload_the_model_list));
            } else {
                final ArrayList<Model> choices = new ArrayList<>(models);
                String[] labels = new String[choices.size()];
                for (int i = 0; i < labels.length; ++i) labels[i] = choices.get(i).label();
                int checked = -1;
                for (int i = 0; i < choices.size(); ++i) if (choices.get(i).id.equals(selectedModelId)) checked = i;
                builder.setSingleChoiceItems(labels, checked, (whichDialog, index) -> {
                    if (closed) return;
                    selectedModelId = choices.get(index).id;
                    prefs.edit().putString("model", selectedModelId).apply();
                    updateControls();
                    whichDialog.dismiss();
                });
            }
            childDialog = builder.create();
            childDialog.show();
        }

        Model selectedModel() {
            for (Model item : models) if (item.id.equals(selectedModelId)) return item;
            return null;
        }
        String selectedEffort() {
            Model item = selectedModel();
            if (item == null) return "";
            String saved = prefs.getString("reasoning:" + item.id, null);
            if (saved == null) return item.recommendedEffort;
            return saved.isEmpty() || item.efforts.contains(saved) ? saved : "";
        }
        String effortLabel(String effort) {
            switch (effort) {
                case "none": return s(R.string.jgchat_none);
                case "minimal": return s(R.string.jgchat_minimal);
                case "low": return s(R.string.jgchat_low);
                case "medium": return s(R.string.jgchat_medium);
                case "high": return s(R.string.jgchat_high);
                case "xhigh": return s(R.string.jgchat_extra_high);
                case "max": return s(R.string.jgchat_max);
                default: return s(R.string.jgchat_server_default);
            }
        }
        void showReasoning() {
            dismissChild();
            Model item = selectedModel();
            AlertDialog.Builder builder = new AlertDialog.Builder(ui).setTitle(s(R.string.jgchat_reasoning_effort))
                    .setNegativeButton(s(R.string.jgchat_close), null);
            if (item == null || item.efforts.isEmpty()) {
                builder.setMessage(s(R.string.jgchat_no_reasoning_options_are_available_for_this_model_the_server_default_will_be_used_relo));
            } else {
                final String modelId = item.id;
                final ArrayList<String> choices = new ArrayList<>();
                choices.add("");
                choices.addAll(item.efforts);
                String[] labels = new String[choices.size()];
                for (int i = 0; i < labels.length; ++i) labels[i] = effortLabel(choices.get(i));
                builder.setSingleChoiceItems(labels, choices.indexOf(selectedEffort()), (whichDialog, index) -> {
                    if (closed) return;
                    prefs.edit().putString("reasoning:" + modelId, choices.get(index)).apply();
                    updateControls();
                    whichDialog.dismiss();
                });
            }
            childDialog = builder.create();
            childDialog.show();
        }
        void dismissChild() {
            keyboardPopup.dismiss();
            if (childDialog != null) { childDialog.dismiss(); childDialog = null; }
            activityDetails = null;
            if (chartDialog != null) { chartDialog.dismiss(); chartDialog = null; }
        }

        String duration(long millis) {
            long seconds = Math.max(0, millis / 1000);
            return (seconds / 60) + ":" + (seconds % 60 < 10 ? "0" : "") + (seconds % 60);
        }
        void showActivity() {
            dismissChild();
            activityDetails = text(activityText);
            activityDetails.setTextIsSelectable(true);
            activityDetails.setPadding(dp(12), dp(8), dp(12), dp(8));
            NestedScrollView scroll = new NestedScrollView(ui);
            scroll.addView(activityDetails, fullWidth());
            childDialog = new AlertDialog.Builder(ui).setTitle(s(R.string.jgchat_current_activity))
                    .setView(scroll).setNegativeButton(s(R.string.jgchat_close), null).create();
            childDialog.setOnDismissListener(ignored -> activityDetails = null);
            childDialog.show();
        }
        void showPlot(int messageIndex, int plotIndex) {
            dismissChild();
            hideKeyboard();
            try {
                String svg = nativeGetPlot(messageIndex, plotIndex);
                if (svg == null || !svg.startsWith("<svg ") || svg.length() > 131072)
                    throw new IllegalStateException(s(R.string.jgchat_the_plot_is_unavailable));
                final String originalSvg = svg;
                if (ColorUtils.calculateLuminance(backgroundColor) <= 0.5)
                    svg = svg.replace("class=\"jg-plot\"", "class=\"jg-plot dark\"");
                final WebView image = new WebView(ui);
                image.setBackgroundColor(backgroundColor);
                image.getSettings().setJavaScriptEnabled(false);
                image.getSettings().setAllowFileAccess(false);
                image.getSettings().setAllowContentAccess(false);
                image.getSettings().setBlockNetworkLoads(true);
                image.getSettings().setBuiltInZoomControls(true);
                image.getSettings().setDisplayZoomControls(false);
                image.getSettings().setUseWideViewPort(true);
                image.getSettings().setLoadWithOverviewMode(true);
                image.setWebViewClient(new WebViewClient() {
                    @Override public boolean shouldOverrideUrlLoading(WebView view, String url) { return true; }
                });
                final AppCompatDialog graph = new AppCompatDialog(ui);
                chartDialog = graph;
                graph.supportRequestWindowFeature(Window.FEATURE_NO_TITLE);
                LinearLayout content = column();
                content.setBackgroundColor(backgroundColor);
                LinearLayout toolbar = new LinearLayout(ui);
                toolbar.setGravity(Gravity.CENTER_VERTICAL);
                toolbar.addView(text(s(R.string.jgchat_plot_pinch_to_zoom)),
                        new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));
                AppCompatButton more = button(s(R.string.jgchat_more));
                more.setOnClickListener(v -> {
                    if (childDialog != null) childDialog.dismiss();
                    childDialog = new AlertDialog.Builder(ui)
                            .setItems(new String[]{s(R.string.jgchat_save_plot), s(R.string.jgchat_share_plot_ellipsis)}, (ignored, which) -> {
                                graph.dismiss(); exportDocument(which == 1, "svg", originalSvg);
                            }).setNegativeButton(s(R.string.jgchat_cancel), null).create();
                    childDialog.show();
                });
                toolbar.addView(more);
                AppCompatButton close = button(s(R.string.jgchat_close));
                close.setOnClickListener(v -> graph.dismiss());
                toolbar.addView(close);
                content.addView(toolbar, fullWidth());
                content.addView(image, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1));
                graph.setContentView(content);
                graph.setOnDismissListener(ignored -> {
                    image.stopLoading(); image.destroy();
                    if (chartDialog == graph) chartDialog = null;
                });
                graph.show();
                Window window = graph.getWindow();
                if (window != null) {
                    window.setBackgroundDrawable(new ColorDrawable(backgroundColor));
                    window.setLayout(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
                    WindowCompat.setDecorFitsSystemWindows(window, false);
                    boolean light = ColorUtils.calculateLuminance(backgroundColor) > 0.5;
                    WindowCompat.getInsetsController(window, content).setAppearanceLightStatusBars(light);
                    WindowCompat.getInsetsController(window, content).setAppearanceLightNavigationBars(light);
                }
                ViewCompat.setOnApplyWindowInsetsListener(content, (view, insets) -> {
                    Insets bars = insets.getInsets(WindowInsetsCompat.Type.systemBars() | WindowInsetsCompat.Type.displayCutout());
                    view.setPadding(bars.left + dp(8), bars.top, bars.right + dp(8), bars.bottom);
                    return WindowInsetsCompat.CONSUMED;
                });
                ViewCompat.requestApplyInsets(content);
                // A local SVG image, not model-authored HTML or a remote page.
                image.loadDataWithBaseURL(null, "<!doctype html><html><head><meta name=\"viewport\" "
                        + "content=\"width=device-width,initial-scale=1\"><meta http-equiv=\"Content-Security-Policy\" "
                        + "content=\"default-src 'none'; style-src 'unsafe-inline'\">"
                        + "<style>html,body{margin:0;height:100%;}svg{width:100%;height:100%;display:block;}</style>"
                        + "</head><body>" + svg + "</body></html>", "text/html", "UTF-8", null);
            } catch (RuntimeException | LinkageError failure) {
                dismissChild(); localError = s(R.string.jgchat_plot_error, message(failure)); showError();
            }
        }

        void showSettings(boolean show) {
            hideKeyboard();
            root.requestFocus();
            settingsOpen = show;
            running.settingsOpen = show;
            updateControls();
        }
        void displayPage(boolean accountPage) {
            if (!opened || closed || helpOpen) return;
            if (accountPage) {
                if (settingsAttached) return;
                hideKeyboard();
                dialog.hide();
                MainActivity.addMyContentView(activity, settingsLayout, new ViewGroup.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
                settingsAttached = true;
                WeakReference<ChatWindow> reference = new WeakReference<>(this);
                MainActivity.setonback(() -> {
                    ChatWindow window = reference.get();
                    if (window != null) window.settingsBack();
                });
                settingsBackDepth = MainActivity.onbacknr();
            } else {
                removeSettings();
                Window window = dialog.getWindow();
                if (!dialog.isShowing() || (window != null && window.getDecorView().getVisibility() != View.VISIBLE)) {
                    dialog.show();
                    resizeDialog();
                }
            }
        }
        void settingsBack() {
            // MainActivity has already popped this callback.
            settingsBackDepth = 0;
            settingsAttached = false;
            tk.glucodata.settings.Settings.removeContentView(settingsLayout);
            if (closed) return;
            boolean loginActive = busy && (!loginUrl.isEmpty() || !userCode.isEmpty());
            if (loggedIn && !loginActive) showSettings(false);
            else dialogClose();
        }
        void removeSettings() {
            if (!settingsAttached) return;
            // Do not pop another Juggluco view which is above Settings.
            if (MainActivity.onbacknr() == settingsBackDepth) MainActivity.poponback();
            settingsBackDepth = 0;
            settingsAttached = false;
            tk.glucodata.settings.Settings.removeContentView(settingsLayout);
        }
        void showHelp() {
            dismissChild();
            hideKeyboard();
            helpOpen = true;
            try {
                help.help(R.string.chatgpthelp, activity, ignored -> {
                    helpOpen = false;
                    updateControls();
                });
            } catch (RuntimeException failure) {
                helpOpen = false;
                updateControls();
                localError = message(failure); showError();
            }
        }
        void hideKeyboard() {
            keyboardPopup.dismiss();
            Window window = dialog.getWindow();
            if (window != null) WindowCompat.getInsetsController(window, root).hide(WindowInsetsCompat.Type.ime());
        }

        boolean allowedLoginUrl(String value) {
            if (value == null || value.isEmpty() || value.indexOf('\n') >= 0 || value.indexOf('\r') >= 0) return false;
            Uri uri = Uri.parse(value);
            return "https".equalsIgnoreCase(uri.getScheme())
                    && "auth.openai.com".equalsIgnoreCase(uri.getHost())
                    && uri.getUserInfo() == null && (uri.getPort() == -1 || uri.getPort() == 443)
                    && ("/codex/device".equals(uri.getPath()) || "/codex/device/".equals(uri.getPath()))
                    && uri.getQuery() == null && uri.getFragment() == null;
        }
        void openLogin() {
            if (!allowedLoginUrl(loginUrl)) {
                localError = s(R.string.jgchat_the_sign_in_address_is_not_the_expected_openai_page);
                showError();
                return;
            }
            try {
                activity.startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(loginUrl)));
            } catch (RuntimeException failure) {
                localError = s(R.string.jgchat_sign_in_error, message(failure));
                showError();
            }
        }
        void copyLoginCode() {
            if (userCode.isEmpty()) return;
            try {
                ClipboardManager clipboard = (ClipboardManager) activity.getSystemService(Context.CLIPBOARD_SERVICE);
                if (clipboard == null) throw new IllegalStateException(s(R.string.jgchat_clipboard_unavailable));
                ClipData clip = ClipData.newPlainText(s(R.string.jgchat_openai_one_time_code), userCode);
                if (Build.VERSION.SDK_INT >= 24) {
                    PersistableBundle extras = new PersistableBundle();
                    extras.putBoolean("android.content.extra.IS_SENSITIVE", true);
                    clip.getDescription().setExtras(extras);
                }
                clipboard.setPrimaryClip(clip);
            } catch (RuntimeException failure) {
                localError = s(R.string.jgchat_copy_code_error, message(failure));
                showError();
            }
        }

        void dialogClose() {
            hideKeyboard();
            running.restoreKeyboard = false;
            // Dismiss only the chat. Juggluco owns the underlying screen and
            // its back stack; never discard unfinished settings on that stack.
            dialog.dismiss();
            close(); // Settings can close before the chat dialog was first shown.
        }
        void close() {
            if (closed) return;
            saveSelection(); saveScroll();
            running.settingsOpen = settingsOpen;
            closed = true;
            removeSettings();
            keyboardPopup.dismiss();
            handler.removeCallbacksAndMessages(null);
            if (chatSurface.getViewTreeObserver().isAlive())
                chatSurface.getViewTreeObserver().removeOnGlobalLayoutListener(chatGeometryListener);
            application.unregisterActivityLifecycleCallbacks(this);
            application.unregisterComponentCallbacks(this);
            dismissChild();
            if (current.get() == this) current.clear();
            running.refresh();
        }
        void configureWindow() {
            Window window = dialog.getWindow();
            if (window == null) return;
            window.clearFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN
                    | WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM | WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE);
            window.setBackgroundDrawable(new ColorDrawable(backgroundColor));
            window.addFlags(WindowManager.LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS);
            window.setStatusBarColor(backgroundColor);
            window.setNavigationBarColor(backgroundColor);
            WindowCompat.setDecorFitsSystemWindows(window, false);
            WindowManager.LayoutParams attributes = window.getAttributes();
            if (Build.VERSION.SDK_INT >= 30) {
                // Neither WindowManager nor our content fits the keyboard.
                // The full-height chat stays behind it, just like an overlay.
                attributes.setFitInsetsTypes(0);
                attributes.layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS;
            } else if (Build.VERSION.SDK_INT >= 28) {
                attributes.layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
            }
            window.setAttributes(attributes);
            boolean light = ColorUtils.calculateLuminance(backgroundColor) > 0.5;
            WindowCompat.getInsetsController(window, root).setAppearanceLightStatusBars(light);
            WindowCompat.getInsetsController(window, root).setAppearanceLightNavigationBars(light);
            window.setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_NOTHING
                    | WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_HIDDEN);
            View decor = window.getDecorView();
            for (View container = root; container != decor; ) {
                container.setFitsSystemWindows(false);
                if (!(container.getParent() instanceof View)) break;
                container = (View) container.getParent();
            }
            chatSurface.getViewTreeObserver().addOnGlobalLayoutListener(chatGeometryListener);
            ViewCompat.setOnApplyWindowInsetsListener(chatSurface, (view, insets) -> {
                // Post until the new insets and rotated window frame agree.
                view.post(this::applyChatGeometry);
                return WindowInsetsCompat.CONSUMED;
            });
        }
        void applyChatGeometry() {
            Window window = dialog.getWindow();
            if (closed || window == null || chatSurface.getWidth() == 0) return;
            View decor = window.getDecorView();
            decor.getLocationOnScreen(chatScreenPosition);
            chatDecorBounds.set(chatScreenPosition[0], chatScreenPosition[1],
                    chatScreenPosition[0] + decor.getWidth(), chatScreenPosition[1] + decor.getHeight());
            root.getLocationOnScreen(chatScreenPosition);
            chatContentBounds.set(chatScreenPosition[0], chatScreenPosition[1],
                    chatScreenPosition[0] + root.getWidth(), chatScreenPosition[1] + root.getHeight());
            WindowInsetsCompat insets = ViewCompat.getRootWindowInsets(decor);
            if (insets == null) return;
            Insets bars;
            if (Build.VERSION.SDK_INT >= 30) {
                android.view.WindowMetrics metrics = window.getWindowManager().getCurrentWindowMetrics();
                chatWindowBounds.set(metrics.getBounds());
                bars = WindowInsetsCompat.toWindowInsetsCompat(metrics.getWindowInsets())
                        .getInsets(WindowInsetsCompat.Type.systemBars());
            } else {
                chatWindowBounds.set(chatDecorBounds);
                bars = insets.getInsets(WindowInsetsCompat.Type.systemBars());
            }
            // Exclude actual system bars only, preserving the one-sided
            // landscape navigation margin. Never include IME in root padding.
            chatAvailableBounds.set(chatWindowBounds.left + bars.left, chatWindowBounds.top + bars.top,
                    chatWindowBounds.right - bars.right, chatWindowBounds.bottom - bars.bottom);
            int left = dp(8) + Math.max(0, chatAvailableBounds.left - chatContentBounds.left);
            int top = Math.max(0, chatAvailableBounds.top - chatContentBounds.top);
            int right = dp(8) + Math.max(0, chatContentBounds.right - chatAvailableBounds.right);
            int bottom = Math.max(0, chatContentBounds.bottom - chatAvailableBounds.bottom);
            if (root.getPaddingLeft() != left || root.getPaddingTop() != top
                    || root.getPaddingRight() != right || root.getPaddingBottom() != bottom)
                root.setPadding(left, top, right, bottom);
            Insets ime = insets.getInsets(WindowInsetsCompat.Type.ime());
            boolean visible = insets.isVisible(WindowInsetsCompat.Type.ime()) && ime.bottom > 0;
            int keyboardTop = chatDecorBounds.bottom - ime.bottom;
            if (Build.VERSION.SDK_INT < 30 && !visible) {
                // Some older keyboards expose their edge only as a visible frame.
                root.getWindowVisibleDisplayFrame(chatVisibleBounds);
                if (!chatVisibleBounds.isEmpty() && chatDecorBounds.contains(chatVisibleBounds)
                        && chatVisibleBounds.bottom < chatAvailableBounds.bottom - dp(80)) {
                    visible = true;
                    keyboardTop = chatVisibleBounds.bottom;
                }
            }
            boolean rtl = activity.getResources().getConfiguration().getLayoutDirection() == View.LAYOUT_DIRECTION_RTL;
            int hideX = 0, hideY = 0, popupX = 0, popupY = 0;
            if (visible && resumed && !settingsAttached && !helpOpen && dialog.isShowing()
                    && (childDialog == null || !childDialog.isShowing())
                    && (chartDialog == null || !chartDialog.isShowing())) {
                int hideLeft = Math.max(chatAvailableBounds.left, chatDecorBounds.left);
                int hideRight = Math.max(hideLeft, Math.min(chatAvailableBounds.right, chatDecorBounds.right));
                keyboardHide.measure(View.MeasureSpec.makeMeasureSpec(hideRight - hideLeft, View.MeasureSpec.AT_MOST),
                        View.MeasureSpec.makeMeasureSpec(0, View.MeasureSpec.UNSPECIFIED));
                int width = keyboardHide.getMeasuredWidth(), height = keyboardHide.getMeasuredHeight();
                popupX = keyboardButtonLeft(width, rtl, hideLeft, hideRight, dp(8), chatDecorBounds.left);
                hideX = chatDecorBounds.left + popupX;
                hideY = keyboardButtonTop(keyboardTop, height, chatWindowBounds.height() >= chatWindowBounds.width(),
                        chatAvailableBounds.top, chatAvailableBounds.bottom);
                popupY = hideY - chatDecorBounds.top;
                try {
                    if (keyboardPopup.isShowing()) keyboardPopup.update(popupX, popupY, width, height);
                    else {
                        keyboardPopup.setWidth(width); keyboardPopup.setHeight(height);
                        // RTL is already resolved above; these are physical offsets.
                        keyboardPopup.showAtLocation(chatSurface, Gravity.TOP | Gravity.LEFT, popupX, popupY);
                    }
                } catch (WindowManager.BadTokenException | IllegalStateException unavailable) {
                    keyboardPopup.dismiss();
                }
            } else keyboardPopup.dismiss();
            String report = "chat geometry overlay v3: decor=" + chatDecorBounds
                    + " content=" + chatContentBounds + " bars=" + bars + " ime=" + ime
                    + " padding=" + left + "," + top + "," + right + "," + bottom
                    + " keyboardVisible=" + visible + " keyboardTop=" + keyboardTop
                    + " hide=" + hideX + "," + hideY + "," + keyboardHide.getMeasuredWidth()
                    + "," + keyboardHide.getMeasuredHeight() + " rtl=" + rtl
                    + " popupOffset=" + popupX + "," + popupY + " popup=" + keyboardPopup.isShowing();
            if (!report.equals(lastGeometryReport)) {
                Log.i("JugglucoChat", report);
                lastGeometryReport = report;
            }
        }
        void resizeDialog() {
            Window window = dialog.getWindow();
            if (window == null || closed) return;
            window.getDecorView().setPadding(0, 0, 0, 0);
            window.setLayout(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
            ViewCompat.requestApplyInsets(chatSurface);
            if (chartDialog != null && chartDialog.getWindow() != null) {
                Window graph = chartDialog.getWindow();
                graph.setLayout(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
                ViewCompat.requestApplyInsets(graph.getDecorView());
            }
        }
        @Override public void onConfigurationChanged(Configuration configuration) {
            if (!closed) resizeDialog();
        }
        @Override public void onLowMemory() {}
        @Override public void onActivityResumed(Activity target) {
            if (target == activity && !closed) { resumed = true; resizeDialog(); refresh(); }
        }
        @Override public void onActivityPaused(Activity target) {
            if (target == activity) {
                saveSelection(); saveScroll();
                resumed = false;
                keyboardPopup.dismiss();
                handler.removeCallbacksAndMessages(null);
                // Monitoring belongs to the application, including while a
                // browser, document picker or Juggluco curve is on screen.
                running.refresh();
            }
        }
        @Override public void onActivityDestroyed(Activity target) {
            if (target == activity) {
                if (!closed && activity.isChangingConfigurations()) {
                    running.reopenAfterRotation = true;
                    WindowInsetsCompat insets = ViewCompat.getRootWindowInsets(root);
                    running.restoreKeyboard = question.hasFocus() && insets != null
                            && insets.isVisible(WindowInsetsCompat.Type.ime());
                }
                close();
                if (dialog.isShowing()) dialog.dismiss();
            }
        }
        @Override public void onActivityCreated(Activity target, Bundle state) {}
        @Override public void onActivityStarted(Activity target) {}
        @Override public void onActivityStopped(Activity target) {}
        @Override public void onActivitySaveInstanceState(Activity target, Bundle state) {}
    }


    /** Native status/errors carry numeric IDs and unformatted arguments.
     * The generated array is indexed directly; English text never selects a resource. */
    @Keep
    static final class NativeMessage {
        final int id, format;
        final String[] args;
        NativeMessage(int id, int format, String[] args) {
            this.id = id; this.format = format; this.args = args == null ? new String[0] : args;
        }
        // BEGIN GENERATED NATIVE RESOURCES
        private static final int[] RESOURCES = {
            0,
            R.string.jgchat_native_a_web_server_secret_consisting_only_of_one_or_two_dots_cannot_be_used_in_a_browser_pa, // 1
            R.string.jgchat_native_aligning_glucose_entered_amounts_and_iob, // 2
            R.string.jgchat_native_ambiguous_https_body_framing, // 3
            R.string.jgchat_native_amount_outside_timeline_bounds, // 4
            R.string.jgchat_native_an_analysis_transaction_is_already_active, // 5
            R.string.jgchat_native_analysis_changes_require_a_transaction, // 6
            R.string.jgchat_native_analysis_columns_must_have_unique_nonempty_names_case_insensitive, // 7
            R.string.jgchat_native_analysis_has_reached_its_saved_file_bundle_limit, // 8
            R.string.jgchat_native_analysis_index_is_full_use_more_clear_analysis_memory, // 9
            R.string.jgchat_native_analysis_input_exceeds_100000_rows_500000_cells_aggregate_smaller_windows_first, // 10
            R.string.jgchat_native_analysis_memory_cleared, // 11
            R.string.jgchat_native_analysis_memory_is_full_64_mib_4096_entries_use_more_clear_analysis_memory, // 12
            R.string.jgchat_native_analysis_needs_one_sql_select_at_most_16_kib_and_at_most_16_tables, // 13
            R.string.jgchat_native_analysis_output_exceeds_20000_rows_2_mib_use_aggregation_or_an_explicit_limit_no_part, // 14
            R.string.jgchat_native_analysis_produced_a_nonfinite_numeric_result, // 15
            R.string.jgchat_native_analysis_reached_its_five_second_20_million_step_budget_narrow_or_aggregate_the_input, // 16
            R.string.jgchat_native_analysis_source_does_not_contain_that_table_path, // 17
            R.string.jgchat_native_analysis_source_snapshots_exceed_16_mib, // 18
            R.string.jgchat_native_analysis_table_exceeds_100000_rows_500000_cells, // 19
            R.string.jgchat_native_analysis_table_exceeds_500000_cells, // 20
            R.string.jgchat_native_analysis_table_exceeds_64_columns, // 21
            R.string.jgchat_native_android_activity_text_too_long, // 22
            R.string.jgchat_native_android_system_ca_certificates_unavailable, // 23
            R.string.jgchat_native_answer_attachments_exceed_the_transcript_limit, // 24
            R.string.jgchat_native_application_storage_directory_must_be_absolute, // 25
            R.string.jgchat_native_array_offset_exceeds_its_size, // 26
            R.string.jgchat_native_at_most_1000_sensor_ids_can_be_matched_at_once, // 27
            R.string.jgchat_native_authentication_cancelled, // 28
            R.string.jgchat_native_authentication_network_request_failed, // 29
            R.string.jgchat_native_authentication_request_failed_http, // 30
            R.string.jgchat_native_binary_sql_results_are_not_supported, // 31
            R.string.jgchat_native_calculating_juggluco_iob, // 32
            R.string.jgchat_native_calculating_juggluco_statistics, // 33
            R.string.jgchat_native_calculating_over_saved_datasets, // 34
            R.string.jgchat_native_calculating_predictions_from_a_saved_model, // 35
            R.string.jgchat_native_cancel_the_current_operation_and_wait_for_it_to_finish, // 36
            R.string.jgchat_native_cancelled, // 37
            R.string.jgchat_native_cannot_allocate_a_result_identifier, // 38
            R.string.jgchat_native_cannot_attach_tls_socket, // 39
            R.string.jgchat_native_cannot_configure_tls_hostname_verification, // 40
            R.string.jgchat_native_cannot_create_private_analysis_storage, // 41
            R.string.jgchat_native_cannot_create_private_chat_state, // 42
            R.string.jgchat_native_cannot_create_private_chat_storage, // 43
            R.string.jgchat_native_cannot_flush_analysis_result, // 44
            R.string.jgchat_native_cannot_format_plot_time, // 45
            R.string.jgchat_native_cannot_format_timeline_time, // 46
            R.string.jgchat_native_cannot_initialize_local_analysis_database, // 47
            R.string.jgchat_native_cannot_open_private_analysis_storage, // 48
            R.string.jgchat_native_cannot_open_private_chat_storage, // 49
            R.string.jgchat_native_cannot_open_saved_chat_state, // 50
            R.string.jgchat_native_cannot_protect_native_tls_write_against_sigpipe, // 51
            R.string.jgchat_native_cannot_read_saved_analysis_result, // 52
            R.string.jgchat_native_cannot_read_saved_chat_state, // 53
            R.string.jgchat_native_cannot_read_text, // 54
            R.string.jgchat_native_cannot_read_the_chat_export, // 55
            R.string.jgchat_native_cannot_remove_saved_chat_state, // 56
            R.string.jgchat_native_cannot_remove_temporary_chat_state, // 57
            R.string.jgchat_native_cannot_replace_temporary_chat_state, // 58
            R.string.jgchat_native_cannot_require_tls_1_2_or_newer, // 59
            R.string.jgchat_native_cannot_restore_analysis_during_a_transaction, // 60
            R.string.jgchat_native_cannot_restrict_private_chat_storage_permissions, // 61
            R.string.jgchat_native_cannot_restrict_saved_chat_state_permissions, // 62
            R.string.jgchat_native_cannot_save_analysis_result, // 63
            R.string.jgchat_native_cannot_save_private_chat_state, // 64
            R.string.jgchat_native_cannot_set_tls_server_name, // 65
            R.string.jgchat_native_cannot_write_analysis_result, // 66
            R.string.jgchat_native_cannot_write_private_chat_state, // 67
            R.string.jgchat_native_cannot_write_the_chat_export, // 68
            R.string.jgchat_native_chat_operation_failed, // 69
            R.string.jgchat_native_chat_state_is_too_large_start_a_new_chat, // 70
            R.string.jgchat_native_chat_storage_has_not_been_initialized, // 71
            R.string.jgchat_native_chat_storage_is_already_initialized_elsewhere, // 72
            R.string.jgchat_native_chat_storage_is_not_initialized, // 73
            R.string.jgchat_native_chatclient_requires_http_tools_and_token_storage, // 74
            R.string.jgchat_native_chatgpt_authentication_failed_sign_in_again, // 75
            R.string.jgchat_native_chatgpt_sign_in_expired_sign_in_again, // 76
            R.string.jgchat_native_checking_completed_response, // 77
            R.string.jgchat_native_checking_stream_gaps_and_backfill, // 78
            R.string.jgchat_native_codex_https_request_failed, // 79
            R.string.jgchat_native_codex_https_request_failed_a923065d, // 80
            R.string.jgchat_native_codex_response_exceeds_the_size_limit, // 81
            R.string.jgchat_native_codex_response_failed, // 82
            R.string.jgchat_native_codex_response_incomplete, // 83
            R.string.jgchat_native_completed_response_contained_an_error, // 84
            R.string.jgchat_native_completed_response_contained_no_final_text_answer, // 85
            R.string.jgchat_native_completed_response_contained_no_output, // 86
            R.string.jgchat_native_completed_response_lacks_an_identifier, // 87
            R.string.jgchat_native_compressed_https_responses_are_unsupported, // 88
            R.string.jgchat_native_conflicting_output_item_events, // 89
            R.string.jgchat_native_connecting, // 90
            R.string.jgchat_native_conversation_exceeds_the_2_mib_history_limit_start_a_new_conversation, // 91
            R.string.jgchat_native_could_not_write_the_complete_chat_export, // 92
            R.string.jgchat_native_delimited_table_has_inconsistent_column_counts, // 93
            R.string.jgchat_native_delimited_table_is_missing_its_header, // 94
            R.string.jgchat_native_device_authorization_expired, // 95
            R.string.jgchat_native_drawing_calculated_results_and_intervals, // 96
            R.string.jgchat_native_drawing_glucose_curve, // 97
            R.string.jgchat_native_drawing_plot, // 98
            R.string.jgchat_native_duplicate_https_content_length, // 99
            R.string.jgchat_native_duplicate_https_request_header, // 100
            R.string.jgchat_native_duplicate_output_item_identifier, // 101
            R.string.jgchat_native_empty_source_has_no_columns_inspect_its_metadata_first, // 102
            R.string.jgchat_native_encrypted_tool_arguments_are_not_supported, // 103
            R.string.jgchat_native_enter_a_question_shorter_than_32_kib, // 104
            R.string.jgchat_native_file_bundle_is_not_referenced_by_this_account_s_workspace, // 105
            R.string.jgchat_native_fitting_and_evaluating_a_numerical_model, // 106
            R.string.jgchat_native_framing_field_in_https_trailer, // 107
            R.string.jgchat_native_function_arguments_exceed_the_size_limit, // 108
            R.string.jgchat_native_function_arguments_must_be_json_text, // 109
            R.string.jgchat_native_function_arguments_must_be_an_object, // 110
            R.string.jgchat_native_https_chunk_line_too_large, // 111
            R.string.jgchat_native_https_header_allocation_failed, // 112
            R.string.jgchat_native_https_library_initialization_failed, // 113
            R.string.jgchat_native_https_request_allocation_failed, // 114
            R.string.jgchat_native_https_request_headers_too_large, // 115
            R.string.jgchat_native_https_response_headers_too_large, // 116
            R.string.jgchat_native_https_response_too_large, // 117
            R.string.jgchat_native_https_trailers_too_large, // 118
            R.string.jgchat_native_https_wire_response_too_large, // 119
            R.string.jgchat_native_history_contains_an_unfinished_turn, // 120
            R.string.jgchat_native_history_contains_an_unpaired_tool_result, // 121
            R.string.jgchat_native_history_contains_duplicate_tool_calls, // 122
            R.string.jgchat_native_history_must_start_with_a_user_message, // 123
            R.string.jgchat_native_incomplete_https_response, // 124
            R.string.jgchat_native_incomplete_output_item, // 125
            R.string.jgchat_native_inconsistent_response_event_type, // 126
            R.string.jgchat_native_increase_the_page_limit_to_fit_one_utf_8_character, // 127
            R.string.jgchat_native_invalid_android_wear_node, // 128
            R.string.jgchat_native_invalid_android_wear_node_text, // 129
            R.string.jgchat_native_invalid_android_wear_snapshot, // 130
            R.string.jgchat_native_invalid_android_activity_field_type, // 131
            R.string.jgchat_native_invalid_android_activity_snapshot, // 132
            R.string.jgchat_native_invalid_android_activity_status, // 133
            R.string.jgchat_native_invalid_https_body_framing, // 134
            R.string.jgchat_native_invalid_https_chunk_extension, // 135
            R.string.jgchat_native_invalid_https_request_configuration, // 136
            R.string.jgchat_native_invalid_https_request_header, // 137
            R.string.jgchat_native_invalid_https_request_path, // 138
            R.string.jgchat_native_invalid_https_response_limit, // 139
            R.string.jgchat_native_invalid_juggluco_tool_arguments, // 140
            R.string.jgchat_native_invalid_activity_count, // 141
            R.string.jgchat_native_invalid_activity_record, // 142
            R.string.jgchat_native_invalid_alarm_settings_counts, // 143
            R.string.jgchat_native_invalid_analysis_row_width, // 144
            R.string.jgchat_native_invalid_authentication_response, // 145
            R.string.jgchat_native_invalid_boolean_local_data_metadata, // 146
            R.string.jgchat_native_invalid_calculation_title_tables, // 147
            R.string.jgchat_native_invalid_chat_export_descriptor, // 148
            R.string.jgchat_native_invalid_chat_state_or_server_response, // 149
            R.string.jgchat_native_invalid_completed_output_item_event, // 150
            R.string.jgchat_native_invalid_conversation_item_identifier, // 151
            R.string.jgchat_native_invalid_delimited_file_quoting, // 152
            R.string.jgchat_native_invalid_file_bundle_identifier, // 153
            R.string.jgchat_native_invalid_file_bundle_use_1_8_text_files_safe_filenames_at_most_48_kib_total_and_an_htm, // 154
            R.string.jgchat_native_invalid_function_call_identifier, // 155
            R.string.jgchat_native_invalid_interim_https_response, // 156
            R.string.jgchat_native_invalid_local_juggluco_response_header, // 157
            R.string.jgchat_native_invalid_local_juggluco_response_status, // 158
            R.string.jgchat_native_invalid_local_file_bundle, // 159
            R.string.jgchat_native_invalid_local_plot_attachment, // 160
            R.string.jgchat_native_invalid_local_tool_definition, // 161
            R.string.jgchat_native_invalid_local_tool_definitions_expected_a_non_empty_array, // 162
            R.string.jgchat_native_invalid_mirror_count, // 163
            R.string.jgchat_native_invalid_native_statistics_envelope, // 164
            R.string.jgchat_native_invalid_native_statistics_response, // 165
            R.string.jgchat_native_invalid_numeric_local_data_metadata, // 166
            R.string.jgchat_native_invalid_or_duplicate_analysis_table_alias, // 167
            R.string.jgchat_native_invalid_output_item_index, // 168
            R.string.jgchat_native_invalid_plot_arguments_check_keys_text_limits_finite_coordinates_and_the_8_series_100, // 169
            R.string.jgchat_native_invalid_plot_metadata, // 170
            R.string.jgchat_native_invalid_plot_row, // 171
            R.string.jgchat_native_invalid_plot_sensor, // 172
            R.string.jgchat_native_invalid_plot_timestamp, // 173
            R.string.jgchat_native_invalid_plot_value, // 174
            R.string.jgchat_native_invalid_private_analysis_storage, // 175
            R.string.jgchat_native_invalid_private_storage_name, // 176
            R.string.jgchat_native_invalid_reasoning_effort, // 177
            R.string.jgchat_native_invalid_saved_analysis_index, // 178
            R.string.jgchat_native_invalid_saved_bundle_identifier, // 179
            R.string.jgchat_native_invalid_saved_result_id, // 180
            R.string.jgchat_native_invalid_saved_result_metadata, // 181
            R.string.jgchat_native_invalid_saved_file_path, // 182
            R.string.jgchat_native_invalid_sensor_count, // 183
            R.string.jgchat_native_invalid_settings_count, // 184
            R.string.jgchat_native_invalid_stream_analysis_bounds, // 185
            R.string.jgchat_native_invalid_stream_count_or_more_than_eight_sensors_shorten_the_interval, // 186
            R.string.jgchat_native_invalid_stream_rate_unavailable_rate_must_be_a_native_nan_token, // 187
            R.string.jgchat_native_invalid_stream_row_count, // 188
            R.string.jgchat_native_invalid_stream_sensor_id, // 189
            R.string.jgchat_native_invalid_stream_timestamp, // 190
            R.string.jgchat_native_invalid_timeline_amount_count, // 191
            R.string.jgchat_native_invalid_timeline_bounds, // 192
            R.string.jgchat_native_invalid_timeline_count_or_more_than_eight_sensors_shorten_the_interval, // 193
            R.string.jgchat_native_invalid_timeline_glucose_value, // 194
            R.string.jgchat_native_invalid_timeline_row_count, // 195
            R.string.jgchat_native_invalid_timeline_sensor, // 196
            R.string.jgchat_native_invalid_timeline_timestamp, // 197
            R.string.jgchat_native_invalid_timeline_unit, // 198
            R.string.jgchat_native_invalid_transcript, // 199
            R.string.jgchat_native_invalid_web_server_secret_configuration, // 200
            R.string.jgchat_native_invalid_working_note, // 201
            R.string.jgchat_native_invalid_workspace_tool_arguments, // 202
            R.string.jgchat_native_json_nesting_limit_exceeded, // 203
            R.string.jgchat_native_json_table_must_be_an_array_with_at_most_100000_rows, // 204
            R.string.jgchat_native_juggluco_settings_are_not_initialized, // 205
            R.string.jgchat_native_listing_reusable_analysis_files, // 206
            R.string.jgchat_native_loading_available_models, // 207
            R.string.jgchat_native_local_juggluco_json_result_must_be_an_object, // 208
            R.string.jgchat_native_local_juggluco_response_length_mismatch, // 209
            R.string.jgchat_native_local_data_response_is_missing_metadata, // 210
            R.string.jgchat_native_local_data_result_exceeds_128_kib_request_a_smaller_period, // 211
            R.string.jgchat_native_looking_up_sensor_in_full_history, // 212
            R.string.jgchat_native_malformed_codex_model_catalog, // 213
            R.string.jgchat_native_malformed_https_chunk_terminator, // 214
            R.string.jgchat_native_malformed_https_header, // 215
            R.string.jgchat_native_malformed_https_headers, // 216
            R.string.jgchat_native_malformed_https_status, // 217
            R.string.jgchat_native_malformed_completed_response_output, // 218
            R.string.jgchat_native_malformed_conversation_item, // 219
            R.string.jgchat_native_malformed_encrypted_context, // 220
            R.string.jgchat_native_malformed_local_juggluco_data_response, // 221
            R.string.jgchat_native_malformed_local_tool_result, // 222
            R.string.jgchat_native_malformed_message_content, // 223
            R.string.jgchat_native_malformed_reasoning_context, // 224
            R.string.jgchat_native_malformed_response_event, // 225
            R.string.jgchat_native_malformed_response_completed_event, // 226
            R.string.jgchat_native_malformed_selectable_model_metadata, // 227
            R.string.jgchat_native_malformed_web_search_action, // 228
            R.string.jgchat_native_missing_android_activity_section, // 229
            R.string.jgchat_native_missing_glucose_sensorid_header, // 230
            R.string.jgchat_native_missing_glucose_sensor_column, // 231
            R.string.jgchat_native_missing_numerical_table_column, // 232
            R.string.jgchat_native_missing_or_invalid_chatgpt_login_sign_in_again, // 233
            R.string.jgchat_native_missing_plot_columns, // 234
            R.string.jgchat_native_missing_timeline_glucose_columns, // 235
            R.string.jgchat_native_missing_workspace_tool_argument, // 236
            R.string.jgchat_native_model_is_working, // 237
            R.string.jgchat_native_model_requested_an_unavailable_tool, // 238
            R.string.jgchat_native_more_than_200_stream_intervals_shorten_the_window_no_partial_analysis_returned, // 239
            R.string.jgchat_native_native_https_dns_resolution_failed, // 240
            R.string.jgchat_native_native_https_tcp_connection_failed, // 241
            R.string.jgchat_native_native_https_cancelled, // 242
            R.string.jgchat_native_native_https_socket_closed, // 243
            R.string.jgchat_native_native_https_socket_poll_failed, // 244
            R.string.jgchat_native_native_https_timed_out, // 245
            R.string.jgchat_native_native_openssl_boringssl_libraries_unavailable, // 246
            R.string.jgchat_native_native_tls_ip_certificate_verification_unavailable, // 247
            R.string.jgchat_native_native_tls_sni_unavailable, // 248
            R.string.jgchat_native_native_tls_connection_allocation_failed, // 249
            R.string.jgchat_native_native_tls_context_allocation_failed, // 250
            R.string.jgchat_native_native_tls_hostname_verification_unavailable, // 251
            R.string.jgchat_native_native_tls_initialization_failed, // 252
            R.string.jgchat_native_native_tls_minimum_version_control_unavailable, // 253
            R.string.jgchat_native_native_tls_read_failed_or_ended_without_close_notify, // 254
            R.string.jgchat_native_no_chat_is_available_to_export, // 255
            R.string.jgchat_native_no_models_returned_reload_models, // 256
            R.string.jgchat_native_numerical_calculation_needs_a_title_and_saved_source, // 257
            R.string.jgchat_native_numerical_source_does_not_contain_that_table_path, // 258
            R.string.jgchat_native_numerical_work_exceeds_10_seconds_60_million_operations_use_fewer_rows_features_targe, // 259
            R.string.jgchat_native_object_scalar_reads_require_offset_zero, // 260
            R.string.jgchat_native_only_notes_can_supersede_notes, // 261
            R.string.jgchat_native_only_one_read_only_select_without_parameters_is_allowed, // 262
            R.string.jgchat_native_open_the_sign_in_page_and_enter_the_code, // 263
            R.string.jgchat_native_operation_failed, // 264
            R.string.jgchat_native_output_item_index_out_of_range, // 265
            R.string.jgchat_native_per_question_limit_of_24_local_data_calls_reached, // 266
            R.string.jgchat_native_per_question_limit_of_25_response_rounds_reached, // 267
            R.string.jgchat_native_per_question_response_round_limit_reached, // 268
            R.string.jgchat_native_plot_count_mismatch, // 269
            R.string.jgchat_native_preparing_a_data_request, // 270
            R.string.jgchat_native_preparing_files, // 271
            R.string.jgchat_native_progress_nesting_limit, // 272
            R.string.jgchat_native_question_exceeded_the_10_minute_time_limit, // 273
            R.string.jgchat_native_question_must_contain_1_65536_bytes, // 274
            R.string.jgchat_native_reading_juggluco_settings, // 275
            R.string.jgchat_native_reading_a_saved_analysis_file, // 276
            R.string.jgchat_native_reading_entered_amounts, // 277
            R.string.jgchat_native_reading_food_composition, // 278
            R.string.jgchat_native_reading_glucose, // 279
            R.string.jgchat_native_reading_ingredient_definitions, // 280
            R.string.jgchat_native_reading_local_data, // 281
            R.string.jgchat_native_reading_meal_contents, // 282
            R.string.jgchat_native_reading_mirrors_and_wear_os_watches, // 283
            R.string.jgchat_native_reading_phone_alarm_settings, // 284
            R.string.jgchat_native_reading_sensor_and_garmin_activity, // 285
            R.string.jgchat_native_reading_sensor_types_and_wear_times, // 286
            R.string.jgchat_native_reading_stream_and_history_calibrations, // 287
            R.string.jgchat_native_reading_units_labels_and_iob_settings, // 288
            R.string.jgchat_native_reading_web_server_capabilities, // 289
            R.string.jgchat_native_ready, // 290
            R.string.jgchat_native_reasoning_effort_is_not_offered_for_this_model_reload_models_or_choose_server_default, // 291
            R.string.jgchat_native_refreshed_login_could_not_be_saved_sign_in_again, // 292
            R.string.jgchat_native_reopening_saved_evidence, // 293
            R.string.jgchat_native_request_cancelled, // 294
            R.string.jgchat_native_reserved_https_request_header, // 295
            R.string.jgchat_native_response_completion_has_a_non_completed_status, // 296
            R.string.jgchat_native_response_data_followed_the_stream_terminator, // 297
            R.string.jgchat_native_response_event_limit_exceeded, // 298
            R.string.jgchat_native_response_is_missing_an_output_item, // 299
            R.string.jgchat_native_response_repeated_a_function_call_identifier, // 300
            R.string.jgchat_native_result_metadata_exceeds_a_page_narrow_the_source_query, // 301
            R.string.jgchat_native_row_exceeds_page_size_use_a_json_pointer_into_that_row, // 302
            R.string.jgchat_native_saved_json_exceeds_the_nesting_limit, // 303
            R.string.jgchat_native_saved_analysis_exceeds_its_capacity, // 304
            R.string.jgchat_native_saved_analysis_result_contains_invalid_json, // 305
            R.string.jgchat_native_saved_chat_state_is_invalid_sign_in_again_or_start_a_new_chat, // 306
            R.string.jgchat_native_saved_chat_state_is_not_a_bounded_regular_file, // 307
            R.string.jgchat_native_saved_chat_state_is_too_large, // 308
            R.string.jgchat_native_saved_conversation_or_analysis_index_could_not_be_read_starting_a_new_chat, // 309
            R.string.jgchat_native_saved_result_does_not_contain_that_json_pointer_path, // 310
            R.string.jgchat_native_saved_result_is_missing_or_invalid_retrieve_the_source_data_again, // 311
            R.string.jgchat_native_saved_result_was_not_found_in_this_account_s_workspace, // 312
            R.string.jgchat_native_saved_sign_in_could_not_be_read_sign_in_again, // 313
            R.string.jgchat_native_saving_generated_files, // 314
            R.string.jgchat_native_saving_glucose_dataset_for_local_analysis, // 315
            R.string.jgchat_native_saving_investigation_notes, // 316
            R.string.jgchat_native_searching_juggluco_food_database, // 317
            R.string.jgchat_native_searching_and_reading_web_pages, // 318
            R.string.jgchat_native_searching_saved_analysis_and_earlier_conversations, // 319
            R.string.jgchat_native_searching_the_internet, // 320
            R.string.jgchat_native_select_a_model, // 321
            R.string.jgchat_native_select_a_valid_model_from_the_returned_model_catalog, // 322
            R.string.jgchat_native_sensor_id_must_be_1_128_bytes_without_nul_tab_or_newline, // 323
            R.string.jgchat_native_sensor_index_must_be_0_1000000_or_null, // 324
            R.string.jgchat_native_sign_in_first, // 325
            R.string.jgchat_native_sign_in_to_ask_a_question, // 326
            R.string.jgchat_native_signed_in, // 327
            R.string.jgchat_native_signed_in_select_a_model, // 328
            R.string.jgchat_native_signed_out, // 329
            R.string.jgchat_native_signed_out_for_this_run_but_saved_credentials_could_not_be_removed_retry_sign_out, // 330
            R.string.jgchat_native_signed_out_but_the_saved_conversation_could_not_be_removed_retry_sign_out, // 331
            R.string.jgchat_native_stream_analysis_requires_stream_data_and_a_1_1440_minute_threshold, // 332
            R.string.jgchat_native_stream_ended_before_response_completed, // 333
            R.string.jgchat_native_system_ca_certificates_unavailable, // 334
            R.string.jgchat_native_tls_certificate_ip_mismatch, // 335
            R.string.jgchat_native_tls_certificate_hostname_mismatch, // 336
            R.string.jgchat_native_tls_certificate_chain_verification_failed, // 337
            R.string.jgchat_native_tls_hostname_verification_unavailable, // 338
            R.string.jgchat_native_tls_peer_certificate_missing, // 339
            R.string.jgchat_native_table_format_must_be_json_csv_or_tsv_and_match_its_source, // 340
            R.string.jgchat_native_text_is_too_long, // 341
            R.string.jgchat_native_text_offset_must_be_a_utf_8_character_boundary_within_the_string, // 342
            R.string.jgchat_native_the_chat_export_is_too_large, // 343
            R.string.jgchat_native_the_file_is_saved_enable_web_server_in_settings_exchange_data_web_server_then_open_it, // 344
            R.string.jgchat_native_the_plot_is_unavailable, // 345
            R.string.jgchat_native_there_are_no_messages_to_export, // 346
            R.string.jgchat_native_thinking, // 347
            R.string.jgchat_native_thinking_summary, // 348
            R.string.jgchat_native_tool_namespaces_are_not_supported, // 349
            R.string.jgchat_native_truncated_response_event, // 350
            R.string.jgchat_native_truncated_response_event_line, // 351
            R.string.jgchat_native_unclosed_delimited_file_quote, // 352
            R.string.jgchat_native_unexpected_https_body_framing, // 353
            R.string.jgchat_native_unexpected_data_after_https_body, // 354
            R.string.jgchat_native_unexpected_data_after_https_response, // 355
            R.string.jgchat_native_unexpected_local_juggluco_response_content_type, // 356
            R.string.jgchat_native_unexpected_sensor_id_in_exact_lookup, // 357
            R.string.jgchat_native_unknown_juggluco_data_tool, // 358
            R.string.jgchat_native_unknown_activity_section, // 359
            R.string.jgchat_native_unknown_analysis_search_kind, // 360
            R.string.jgchat_native_unknown_settings_section, // 361
            R.string.jgchat_native_unknown_workspace_tool, // 362
            R.string.jgchat_native_unsigned_value_exceeds_sql_integer_range, // 363
            R.string.jgchat_native_unsupported_codex_endpoint, // 364
            R.string.jgchat_native_unsupported_https_content_encoding, // 365
            R.string.jgchat_native_unsupported_https_request_method, // 366
            R.string.jgchat_native_unsupported_https_status, // 367
            R.string.jgchat_native_unsupported_https_transfer_encoding, // 368
            R.string.jgchat_native_unsupported_chat_export_format, // 369
            R.string.jgchat_native_unsupported_conversation_role, // 370
            R.string.jgchat_native_unsupported_message_content, // 371
            R.string.jgchat_native_unsupported_message_phase, // 372
            R.string.jgchat_native_unsupported_native_statistics_schema, // 373
            R.string.jgchat_native_unsupported_output_item_type, // 374
            R.string.jgchat_native_workspace_argument_must_be_text, // 375
            R.string.jgchat_native_workspace_numeric_argument_is_outside_its_bounds, // 376
            R.string.jgchat_native_workspace_text_argument_exceeds_its_limit, // 377
            R.string.jgchat_native_writing_reply, // 378
            R.string.jgchat_native_external_detail, // 379
            R.string.jgchat_waiting_round, // 380
            R.string.jgchat_native_codex_http, // 381
            R.string.jgchat_native_authentication_http, // 382
            R.string.jgchat_native_no_selectable_models, // 383
            R.string.jgchat_native_tls_function_unavailable, // 384
            R.string.jgchat_native_missing_numerical_column, // 385
            R.string.jgchat_native_analysis_sql, // 386
            R.string.jgchat_native_invalid_tool_arguments, // 387
            R.string.jgchat_native_completed_response_error, // 388
            R.string.jgchat_native_native_tls_handshake_or_certificate_chain_verification_failed, // 389
            R.string.jgchat_native_native_tls_write_failed, // 390
            R.string.jgchat_native_model_id_must_identify_a_saved_juggluco_fit_result, // 391
            R.string.jgchat_native_malformed_function_arguments, // 392
            R.string.jgchat_native_malformed_response_event_json, // 393
            R.string.jgchat_native_invalid_error_response, // 394
            R.string.jgchat_native_cannot_create_generated_file_directory, // 395
            R.string.jgchat_native_cannot_open_generated_file_directory, // 396
            R.string.jgchat_native_cannot_create_generated_file, // 397
            R.string.jgchat_native_cannot_write_generated_file, // 398
            R.string.jgchat_native_cannot_flush_generated_file, // 399
            R.string.jgchat_native_cannot_read_saved_file, // 400
            R.string.jgchat_native_cannot_prepare_document_write, // 401
            R.string.jgchat_native_cannot_generate_file_identifier, // 402
            R.string.jgchat_native_invalid_app_files_directory, // 403
            R.string.jgchat_native_cannot_open_app_files_directory, // 404
            R.string.jgchat_native_saved_file_limit_reached_delete_old_bundles_under_web_server_upload_web_pages_chatgpt, // 405
            R.string.jgchat_native_cannot_create_new_file_bundle, // 406
            R.string.jgchat_native_cannot_open_new_file_bundle, // 407
            R.string.jgchat_native_generated_file_identifier_collision, // 408
            R.string.jgchat_native_cannot_commit_generated_files, // 409
            R.string.jgchat_native_cannot_list_saved_files, // 410
            R.string.jgchat_native_invalid_saved_file, // 411
            R.string.jgchat_native_the_saved_file_is_no_longer_available, // 412
            R.string.jgchat_native_invalid_saved_file_manifest, // 413
            R.string.jgchat_native_file_is_not_part_of_this_generated_bundle, // 414
            R.string.jgchat_native_document_is_not_writable, // 415
            R.string.jgchat_native_could_not_write_the_selected_document, // 416
            R.string.jgchat_native_could_not_check_the_selected_document, // 417
            R.string.jgchat_native_could_not_finish_writing_the_selected_document, // 418
            R.string.jgchat_native_invalid_numerical_tool_arguments, // 419
            R.string.jgchat_native_missing_numerical_tool_argument, // 420
            R.string.jgchat_native_numerical_column_name_argument_must_be_text, // 421
            R.string.jgchat_native_invalid_numerical_column_name_length, // 422
            R.string.jgchat_native_numerical_data_must_be_finite_json_numbers_within_1e100_prepare_csv_tsv_with_explicit, // 423
            R.string.jgchat_native_numerical_overflow_or_unstable_model_rescale_inputs_or_simplify_the_model, // 424
            R.string.jgchat_native_invalid_numerical_feature_target_count, // 425
            R.string.jgchat_native_duplicate_numerical_column, // 426
            R.string.jgchat_native_numerical_tables_require_1_10000_rows_and_at_most_64_columns, // 427
            R.string.jgchat_native_invalid_numerical_table_row, // 428
            R.string.jgchat_native_invalid_saved_numerical_vector, // 429
            R.string.jgchat_native_rank_deficient_or_ill_conditioned_fit_remove_redundant_constant_features_or_use_ridge, // 430
            R.string.jgchat_native_unsupported_saved_numerical_model, // 431
            R.string.jgchat_native_unsupported_saved_numerical_method, // 432
            R.string.jgchat_native_invalid_saved_feature_scaling, // 433
            R.string.jgchat_native_invalid_saved_neighbors, // 434
            R.string.jgchat_native_invalid_saved_neighbor_count, // 435
            R.string.jgchat_native_invalid_saved_coefficients, // 436
            R.string.jgchat_native_numerical_output_exceeds_20000_rows_2_mib_reduce_rows_or_targets_no_partial_result_sa, // 437
            R.string.jgchat_native_a_target_cannot_also_be_a_predictor, // 438
            R.string.jgchat_native_a_target_cannot_serve_as_its_own_baseline, // 439
            R.string.jgchat_native_train_before_must_be_less_than_calibrate_before, // 440
            R.string.jgchat_native_coverage_must_be_0_5_0_99_or_null, // 441
            R.string.jgchat_native_ridge_needs_lambda_in_0_1000000_and_neighbors_null, // 442
            R.string.jgchat_native_knn_needs_predictors_neighbors_1_100_and_lambda_null, // 443
            R.string.jgchat_native_linear_needs_lambda_null_and_neighbors_null, // 444
            R.string.jgchat_native_label_end_column_precedes_its_origin_order_correct_target_availability_times, // 445
            R.string.jgchat_native_too_few_complete_training_rows_after_purging_linear_needs_more_than_features_1_every_, // 446
            R.string.jgchat_native_evaluation_exceeds_20000_rows_use_fewer_rows_or_targets, // 447
            R.string.jgchat_native_invalid_saved_uncertainty_calibration, // 448
            R.string.jgchat_native_invalid_saved_interval_radius, // 449
            R.string.jgchat_native_prediction_exceeds_20000_output_rows_reduce_input_rows, // 450
            R.string.jgchat_native_max_gap_must_be_positive_or_null, // 451
            R.string.jgchat_native_plot_needs_1_8_column_series, // 452
            R.string.jgchat_native_line_band_table_x_must_increase_strictly_order_filter_pivot_it_with_sql, // 453
            R.string.jgchat_native_saved_table_plot_exceeds_1000_total_points_explicitly_reduce_aggregate_in_sql, // 454
            R.string.jgchat_native_codex_http_detail, // 455
            R.string.jgchat_native_https_cancelled, // 456
            R.string.jgchat_native_https_response_processing_failed, // 457
            R.string.jgchat_native_https_transport_failed, // 458
            R.string.jgchat_native_https_unexpected_failure, // 459
            R.string.jgchat_native_native_https_unexpected_failure, // 460
        };
        // END GENERATED NATIVE RESOURCES
        static int resourceId(int id) {
            return id > 0 && id < RESOURCES.length ? RESOURCES[id] : R.string.jgchat_native_chat_operation_failed;
        }
        String text(Context context) {
            try {
                int resource = resourceId(id);
                if (id <= 0 || id >= RESOURCES.length) return context.getString(resource);
                if (format == 1 && args.length == 1)
                    return context.getString(R.string.jgchat_label_detail, context.getString(resource), args[0]);
                if (format == 2 && args.length == 2)
                    return context.getString(R.string.jgchat_progress_range, context.getString(resource), args[0], args[1]);
                if (format != 0) return context.getString(R.string.jgchat_native_chat_operation_failed);
                return args.length == 0 ? context.getString(resource) : context.getString(resource, (Object[]) args);
            } catch (java.util.IllegalFormatException malformedTranslation) {
                return context.getString(R.string.jgchat_native_chat_operation_failed);
            }
        }
        static String text(Context context, JSONObject object) {
            if (object == null) return "";
            JSONArray values = object.optJSONArray("args");
            int count = values == null ? 0 : values.length();
            if (count > 8) return context.getString(R.string.jgchat_native_chat_operation_failed);
            String[] args = new String[count];
            for (int i = 0; i < count; ++i) args[i] = values.optString(i, "");
            return new NativeMessage(object.optInt("id"), object.optInt("format"), args).text(context);
        }
        static String exportLabels(Context context) throws JSONException {
            JSONObject labels = new JSONObject();
            Configuration config = context.getResources().getConfiguration();
            @SuppressWarnings("deprecation")
            Locale locale = Build.VERSION.SDK_INT >= 24 ? config.getLocales().get(0) : config.locale;
            labels.put("language", locale.toLanguageTag());
            labels.put("title", context.getString(R.string.jgchat_export_title));
            labels.put("you", context.getString(R.string.jgchat_you));
            labels.put("assistant", context.getString(R.string.jgchat_chatgpt));
            labels.put("source", context.getString(R.string.jgchat_export_source));
            labels.put("plot", context.getString(R.string.jgchat_plot));
            labels.put("plot_unavailable", context.getString(R.string.jgchat_export_plot_unavailable));
            labels.put("plot_text_note", context.getString(R.string.jgchat_export_plot_text_note));
            labels.put("generated_files", context.getString(R.string.jgchat_export_generated_files));
            labels.put("separate_contents", context.getString(R.string.jgchat_export_separate_contents));
            labels.put("pending", context.getString(R.string.jgchat_export_pending));
            labels.put("incomplete", context.getString(R.string.jgchat_export_incomplete));
            labels.put("answer_heading", context.getString(R.string.jgchat_answer_heading));
            labels.put("effort_default", context.getString(R.string.jgchat_reasoning_default_short));
            labels.put("effort_none", context.getString(R.string.jgchat_reasoning_none_short));
            labels.put("effort_minimal", context.getString(R.string.jgchat_reasoning_minimal_short));
            labels.put("effort_low", context.getString(R.string.jgchat_reasoning_low_short));
            labels.put("effort_medium", context.getString(R.string.jgchat_reasoning_medium_short));
            labels.put("effort_high", context.getString(R.string.jgchat_reasoning_high_short));
            labels.put("effort_xhigh", context.getString(R.string.jgchat_reasoning_xhigh_short));
            labels.put("effort_max", context.getString(R.string.jgchat_reasoning_max_short));
            return labels.toString();
        }
    }
}
