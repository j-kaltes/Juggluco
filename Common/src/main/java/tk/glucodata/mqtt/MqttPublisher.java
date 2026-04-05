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
/*      MQTT Integration - Publisher                                                 */

package tk.glucodata.mqtt;

import android.content.Context;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import org.eclipse.paho.android.service.MqttAndroidClient;
import org.eclipse.paho.client.mqttv3.IMqttActionListener;
import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.IMqttToken;
import org.eclipse.paho.client.mqttv3.MqttCallback;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

import tk.glucodata.Log;

import static tk.glucodata.Applic.app;
import static tk.glucodata.Log.doLog;

/**
 * Manages MQTT connection and publishes glucose readings to an MQTT broker.
 * Implements singleton pattern with automatic reconnection.
 */
public class MqttPublisher {
    private static final String LOG_ID = "MqttPublisher";
    private static final int MAX_QUEUE_SIZE = 100;
    private static final int MAX_RECONNECT_DELAY_MS = 60000; // 1 minute
    private static final int INITIAL_RECONNECT_DELAY_MS = 1000; // 1 second

    private static MqttPublisher instance;
    
    private final Context context;
    private final MqttConfig config;
    private MqttAndroidClient mqttClient;
    
    private final AtomicBoolean isConnected = new AtomicBoolean(false);
    private final AtomicBoolean isConnecting = new AtomicBoolean(false);
    private final AtomicInteger reconnectDelay = new AtomicInteger(INITIAL_RECONNECT_DELAY_MS);
    private final LinkedBlockingQueue<PendingMessage> messageQueue = new LinkedBlockingQueue<>(MAX_QUEUE_SIZE);
    
    private ConnectionStatusListener statusListener;

    /**
     * Listener for connection status changes.
     */
    public interface ConnectionStatusListener {
        void onConnected();
        void onDisconnected();
        void onError(String error);
    }

    /**
     * Represents a pending message to be published.
     */
    private static class PendingMessage {
        final String topic;
        final String payload;
        final int qos;
        final boolean retained;

        PendingMessage(String topic, String payload, int qos, boolean retained) {
            this.topic = topic;
            this.payload = payload;
            this.qos = qos;
            this.retained = retained;
        }
    }

    private MqttPublisher(Context context) {
        this.context = context.getApplicationContext();
        this.config = new MqttConfig(this.context);
    }

    /**
     * Gets the singleton instance.
     */
    public static synchronized MqttPublisher getInstance() {
        if (instance == null) {
            instance = new MqttPublisher(app);
        }
        return instance;
    }

    /**
     * Sets the connection status listener.
     */
    public void setStatusListener(@Nullable ConnectionStatusListener listener) {
        this.statusListener = listener;
    }

    /**
     * Connects to the MQTT broker.
     */
    public synchronized void connect() {
        if (isConnected.get() || isConnecting.get()) {
            if (doLog) Log.i(LOG_ID, "Already connected or connecting");
            return;
        }

        String validationError = config.validate();
        if (validationError != null) {
            if (doLog) Log.e(LOG_ID, "Configuration error: " + validationError);
            notifyError("Configuration error: " + validationError);
            return;
        }

        isConnecting.set(true);
        
        try {
            String brokerUri = config.getBrokerUri();
            String clientId = config.getClientId();
            
            if (doLog) Log.i(LOG_ID, "Connecting to " + brokerUri + " with client ID: " + clientId);
            
            mqttClient = new MqttAndroidClient(context, brokerUri, clientId);
            mqttClient.setCallback(new MqttCallbackHandler());
            
            MqttConnectOptions options = new MqttConnectOptions();
            options.setCleanSession(true);
            options.setAutomaticReconnect(false); // We handle reconnection manually
            options.setConnectionTimeout(30);
            options.setKeepAliveInterval(60);
            
            // Set authentication if configured
            if (config.hasAuthentication()) {
                options.setUserName(config.getUsername());
                String password = config.getPassword();
                if (password != null) {
                    options.setPassword(password.toCharArray());
                }
            }
            
            mqttClient.connect(options, null, new IMqttActionListener() {
                @Override
                public void onSuccess(IMqttToken asyncActionToken) {
                    isConnecting.set(false);
                    isConnected.set(true);
                    reconnectDelay.set(INITIAL_RECONNECT_DELAY_MS);
                    
                    if (doLog) Log.i(LOG_ID, "Connected successfully");
                    notifyConnected();
                    
                    // Publish any queued messages
                    publishQueuedMessages();
                }

                @Override
                public void onFailure(IMqttToken asyncActionToken, Throwable exception) {
                    isConnecting.set(false);
                    isConnected.set(false);
                    
                    String error = "Connection failed: " + exception.getMessage();
                    if (doLog) Log.e(LOG_ID, error);
                    notifyError(error);
                    
                    // Schedule reconnection
                    scheduleReconnect();
                }
            });
            
        } catch (MqttException e) {
            isConnecting.set(false);
            isConnected.set(false);
            
            String error = "MQTT exception: " + e.getMessage();
            if (doLog) Log.e(LOG_ID, error);
            notifyError(error);
            
            scheduleReconnect();
        }
    }

    /**
     * Disconnects from the MQTT broker.
     */
    public synchronized void disconnect() {
        if (mqttClient != null && mqttClient.isConnected()) {
            try {
                if (doLog) Log.i(LOG_ID, "Disconnecting");
                mqttClient.disconnect();
                isConnected.set(false);
                notifyDisconnected();
            } catch (MqttException e) {
                if (doLog) Log.e(LOG_ID, "Error disconnecting: " + e.getMessage());
            }
        }
        
        if (mqttClient != null) {
            try {
                mqttClient.close();
            } catch (Exception e) {
                if (doLog) Log.e(LOG_ID, "Error closing client: " + e.getMessage());
            }
            mqttClient = null;
        }
        
        isConnected.set(false);
        isConnecting.set(false);
    }

    /**
     * Publishes glucose data to MQTT broker.
     */
    public void publishGlucose(@NonNull String serialNumber, double glucose, String unit,
                               long timestamp, float rate, String rateUnit,
                               int alarm, long sensorStartTime, int sensorGen) {
        try {
            JSONObject json = new JSONObject();
            json.put("glucose", glucose);
            json.put("unit", unit);
            json.put("timestamp", timestamp);
            json.put("rate", rate);
            json.put("rateUnit", rateUnit);
            json.put("alarm", alarm);
            json.put("sensorSerial", serialNumber);
            json.put("sensorStartTime", sensorStartTime);
            json.put("sensorGen", sensorGen);
            
            String payload = json.toString();
            String topic = config.getGlucoseTopic();
            int qos = config.getQos();
            boolean retained = config.isRetain();
            
            if (doLog) Log.i(LOG_ID, "Publishing glucose: " + glucose + " " + unit + " to " + topic);
            
            publish(topic, payload, qos, retained);
            
        } catch (JSONException e) {
            if (doLog) Log.e(LOG_ID, "Error creating JSON: " + e.getMessage());
        }
    }

    /**
     * Publishes a message to the MQTT broker.
     */
    private void publish(String topic, String payload, int qos, boolean retained) {
        if (!isConnected.get()) {
            // Queue message for later
            if (messageQueue.size() < MAX_QUEUE_SIZE) {
                messageQueue.offer(new PendingMessage(topic, payload, qos, retained));
                if (doLog) Log.i(LOG_ID, "Message queued (not connected)");
            } else {
                if (doLog) Log.w(LOG_ID, "Message queue full, dropping message");
            }
            
            // Try to connect if not already connecting
            if (!isConnecting.get()) {
                connect();
            }
            return;
        }

        try {
            MqttMessage message = new MqttMessage(payload.getBytes());
            message.setQos(qos);
            message.setRetained(retained);
            
            mqttClient.publish(topic, message, null, new IMqttActionListener() {
                @Override
                public void onSuccess(IMqttToken asyncActionToken) {
                    if (doLog) Log.v(LOG_ID, "Message published successfully");
                }

                @Override
                public void onFailure(IMqttToken asyncActionToken, Throwable exception) {
                    if (doLog) Log.e(LOG_ID, "Publish failed: " + exception.getMessage());
                    
                    // Queue for retry if not too many queued
                    if (messageQueue.size() < MAX_QUEUE_SIZE) {
                        messageQueue.offer(new PendingMessage(topic, payload, qos, retained));
                    }
                }
            });
            
        } catch (MqttException e) {
            if (doLog) Log.e(LOG_ID, "Error publishing: " + e.getMessage());
            
            // Queue for retry
            if (messageQueue.size() < MAX_QUEUE_SIZE) {
                messageQueue.offer(new PendingMessage(topic, payload, qos, retained));
            }
        }
    }

    /**
     * Publishes all queued messages.
     */
    private void publishQueuedMessages() {
        if (doLog) Log.i(LOG_ID, "Publishing " + messageQueue.size() + " queued messages");
        
        PendingMessage msg;
        while ((msg = messageQueue.poll()) != null) {
            publish(msg.topic, msg.payload, msg.qos, msg.retained);
        }
    }

    /**
     * Schedules a reconnection attempt with exponential backoff.
     */
    private void scheduleReconnect() {
        int delay = reconnectDelay.get();
        
        if (doLog) Log.i(LOG_ID, "Scheduling reconnect in " + delay + "ms");
        
        new android.os.Handler(android.os.Looper.getMainLooper()).postDelayed(() -> {
            if (!isConnected.get() && !isConnecting.get()) {
                connect();
            }
        }, delay);
        
        // Increase delay for next time (exponential backoff)
        int nextDelay = Math.min(delay * 2, MAX_RECONNECT_DELAY_MS);
        reconnectDelay.set(nextDelay);
    }

    /**
     * Checks if currently connected.
     */
    public boolean isConnected() {
        return isConnected.get();
    }

    /**
     * Gets the current configuration.
     */
    public MqttConfig getConfig() {
        return config;
    }

    // Notification methods
    private void notifyConnected() {
        if (statusListener != null) {
            statusListener.onConnected();
        }
    }

    private void notifyDisconnected() {
        if (statusListener != null) {
            statusListener.onDisconnected();
        }
    }

    private void notifyError(String error) {
        if (statusListener != null) {
            statusListener.onError(error);
        }
    }

    /**
     * MQTT callback handler for connection events.
     */
    private class MqttCallbackHandler implements MqttCallback {
        @Override
        public void connectionLost(Throwable cause) {
            isConnected.set(false);
            
            String error = "Connection lost: " + (cause != null ? cause.getMessage() : "Unknown");
            if (doLog) Log.w(LOG_ID, error);
            notifyDisconnected();
            
            // Schedule reconnection
            scheduleReconnect();
        }

        @Override
        public void messageArrived(String topic, MqttMessage message) throws Exception {
            // We don't subscribe to any topics, so this shouldn't be called
            if (doLog) Log.v(LOG_ID, "Unexpected message arrived on topic: " + topic);
        }

        @Override
        public void deliveryComplete(IMqttDeliveryToken token) {
            if (doLog) Log.v(LOG_ID, "Delivery complete");
        }
    }
}

// Made with Bob
