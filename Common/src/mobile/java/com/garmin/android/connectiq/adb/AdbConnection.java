package com.garmin.android.connectiq.adb;

import android.content.Context;
import android.util.Log;
import com.garmin.android.connectiq.IQDevice;
import java.io.BufferedInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketTimeoutException;
import java.util.Arrays;

/* loaded from: classes.jar:com/garmin/android/connectiq/adb/AdbConnection.class */
public class AdbConnection {
    public static final int DEFAULT_PORT = 7381;
    private static AdbConnection instance;
    private static final int MAX_BUFFER_SIZE = 16384;
    private Context context;
    private ServerSocket server;
    private Socket client = null;
    private int connectionPort = DEFAULT_PORT;
    private InputStream inputStream = null;
    private OutputStream outputStream = null;
    private Thread serverThread = null;
     AdbConnectionListener connListener = null;
    private IQDevice device = new IQDevice(12345, "Simulator");

    /* loaded from: classes.jar:com/garmin/android/connectiq/adb/AdbConnection$AdbConnectionListener.class */
    public interface AdbConnectionListener {
        void onConnectionStatusChanged(IQDevice iQDevice, IQDevice.IQDeviceStatus iQDeviceStatus);
    }

    public static AdbConnection getInstance() {
        if (instance == null) {
            instance = new AdbConnection();
        }
        return instance;
    }

    private AdbConnection() {
    }

    /* JADX INFO: Access modifiers changed from: package-private */
    /* loaded from: classes.jar:com/garmin/android/connectiq/adb/AdbConnection$ServerThread.class */
    public class ServerThread implements Runnable {
        ServerThread() {
        }

        @Override // java.lang.Runnable
        public void run() {
            while (!Thread.currentThread().isInterrupted()) {
                try {
                    try {
                        AdbConnection.this.server = new ServerSocket(AdbConnection.this.connectionPort);
                        AdbConnection.this.server.setSoTimeout(5000);
                        Log.d("ConnectIQ-AdbConnection", "Waiting for simulator connection.");
                        while (!Thread.currentThread().isInterrupted()) {
                            try {
                                AdbConnection.this.client = AdbConnection.this.server.accept();
                                break;
                            } catch (SocketTimeoutException e) {
                            }
                        }
                        AdbConnection.this.inputStream = new BufferedInputStream(AdbConnection.this.client.getInputStream());
                        AdbConnection.this.outputStream = AdbConnection.this.client.getOutputStream();
                        Log.d("ConnectIQ-AdbConnection", "Simulator connected");
                        if (AdbConnection.this.connListener != null) {
                            AdbConnection.this.connListener.onConnectionStatusChanged(AdbConnection.this.device, IQDevice.IQDeviceStatus.CONNECTED);
                        }
                        try {
                            AdbConnection.this.server.close();
                        } catch (IOException e2) {
                        }
                    } catch (IOException e3) {
                        Log.e("ConnectIQ-AdbConnection", "Error creating server socket", e3);
                        try {
                            AdbConnection.this.server.close();
                        } catch (IOException e4) {
                        }
                    }
                    if (AdbConnection.this.client != null) {
                        byte[] buffer = new byte[AdbConnection.MAX_BUFFER_SIZE];
                        while (true) {
                            if (Thread.currentThread().isInterrupted() || !AdbConnection.this.client.isConnected() || AdbConnection.this.client.isInputShutdown()) {
                                break;
                            }
                            try {
                                Log.d("ConnectIQ-AdbConnection", "Looking for input from simulator.");
                                int bytesRead = AdbConnection.this.inputStream.read(buffer, 0, buffer.length);
                                if (bytesRead == -1) {
                                    Log.d("ConnectIQ-AdbConnection", "End of input reached");
                                    break;
                                }
                                Log.d("ConnectIQ-AdbConnection", "Received " + bytesRead + " from simulator");
                                byte[] data = Arrays.copyOf(buffer, bytesRead);
                                AdbDataHandler.handleMessage(AdbConnection.this.context, data);
                            } catch (IOException e5) {
                                Log.e("ConnectIQ-AdbConnection", "Error reading input stream", e5);
                            }
                        }
                        Log.d("ConnectIQ-AdbConnection", "Client connection shut down.");
                        if (AdbConnection.this.connListener != null) {
                            AdbConnection.this.connListener.onConnectionStatusChanged(AdbConnection.this.device, IQDevice.IQDeviceStatus.NOT_CONNECTED);
                        }
                        try {
                            AdbConnection.this.client.close();
                            AdbConnection.this.client = null;
                        } catch (IOException e6) {
                        }
                    }
                } catch (Throwable th) {
                    try {
                        AdbConnection.this.server.close();
                    } catch (IOException e7) {
                    }
                    throw th;
                }
            }
        }
    }

    public void setPort(int port) {
        this.connectionPort = port;
    }

    public int getPort() {
        return this.connectionPort;
    }

    public void setContext(Context context) {
        this.context = context;
    }

    public boolean isConnected() {
        return this.client != null && this.client.isConnected();
    }

    public boolean isActive() {
        return this.serverThread != null && this.serverThread.isAlive();
    }

    public void establishConnection(AdbConnectionListener listener) {
        this.connListener = listener;
        this.serverThread = new Thread(new ServerThread());
        this.serverThread.start();
    }

    public void disconnect() {
        if (this.client != null) {
            try {
                this.client.close();
            } catch (IOException e) {
            }
        }
        if (this.serverThread != null) {
            this.serverThread.interrupt();
        }
    }

    public boolean sendMessage(byte[] data) {
        if (this.outputStream == null || this.client == null || !this.client.isConnected() || this.client.isOutputShutdown()) {
            return false;
        }
        long delay = (data.length / 600L) * 1000L;
        if (delay > 0) {
            try {
                Log.d("ConnectIQ-AdbConnection", "Simulating BLE, sleeping " + delay + " milliseconds");
                Thread.sleep(delay);
            } catch (InterruptedException e) {
            }
        }
        try {
            this.outputStream.write(data);
            this.outputStream.flush();
            Log.d("ConnectIQ-AdbConnection", "Wrote " + data.length + " bytes to output stream");
            return true;
        } catch (IOException e2) {
            Log.e("ConnectIQ-AdbConnection", "Error sending message", e2);
            return false;
        }
    }
}
