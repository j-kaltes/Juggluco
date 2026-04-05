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
/*      MQTT Integration - Configuration Management                                  */

package tk.glucodata.mqtt;

import android.content.Context;
import android.content.SharedPreferences;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import static tk.glucodata.Applic.app;

/**
 * Manages MQTT broker configuration settings.
 * Stores connection parameters in SharedPreferences.
 */
public class MqttConfig {
    private static final String PREFS_NAME = "mqtt_config";
    private static final String KEY_BROKER_URL = "broker_url";
    private static final String KEY_PORT = "port";
    private static final String KEY_USERNAME = "username";
    private static final String KEY_PASSWORD = "password";
    private static final String KEY_TOPIC_PREFIX = "topic_prefix";
    private static final String KEY_QOS = "qos";
    private static final String KEY_RETAIN = "retain";
    private static final String KEY_USE_WEBSOCKET = "use_websocket";
    private static final String KEY_CLIENT_ID = "client_id";

    // Default values
    private static final String DEFAULT_BROKER_URL = "tcp://localhost";
    private static final int DEFAULT_PORT_TCP = 1883;
    private static final int DEFAULT_PORT_WS = 9001;
    private static final String DEFAULT_TOPIC_PREFIX = "juggluco";
    private static final int DEFAULT_QOS = 1;
    private static final boolean DEFAULT_RETAIN = false;
    private static final boolean DEFAULT_USE_WEBSOCKET = false;

    private final SharedPreferences prefs;

    public MqttConfig(Context context) {
        this.prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE);
    }

    public static MqttConfig getInstance() {
        return new MqttConfig(app);
    }

    // Getters
    @NonNull
    public String getBrokerUrl() {
        return prefs.getString(KEY_BROKER_URL, DEFAULT_BROKER_URL);
    }

    public int getPort() {
        int defaultPort = isUseWebSocket() ? DEFAULT_PORT_WS : DEFAULT_PORT_TCP;
        return prefs.getInt(KEY_PORT, defaultPort);
    }

    @Nullable
    public String getUsername() {
        return prefs.getString(KEY_USERNAME, null);
    }

    @Nullable
    public String getPassword() {
        return prefs.getString(KEY_PASSWORD, null);
    }

    @NonNull
    public String getTopicPrefix() {
        return prefs.getString(KEY_TOPIC_PREFIX, DEFAULT_TOPIC_PREFIX);
    }

    public int getQos() {
        return prefs.getInt(KEY_QOS, DEFAULT_QOS);
    }

    public boolean isRetain() {
        return prefs.getBoolean(KEY_RETAIN, DEFAULT_RETAIN);
    }

    public boolean isUseWebSocket() {
        return prefs.getBoolean(KEY_USE_WEBSOCKET, DEFAULT_USE_WEBSOCKET);
    }

    @NonNull
    public String getClientId() {
        String clientId = prefs.getString(KEY_CLIENT_ID, null);
        if (clientId == null || clientId.isEmpty()) {
            clientId = "juggluco_" + android.os.Build.MODEL.replaceAll("[^a-zA-Z0-9]", "_") + "_" + System.currentTimeMillis();
            setClientId(clientId);
        }
        return clientId;
    }

    // Setters
    public void setBrokerUrl(@NonNull String url) {
        prefs.edit().putString(KEY_BROKER_URL, url).apply();
    }

    public void setPort(int port) {
        prefs.edit().putInt(KEY_PORT, port).apply();
    }

    public void setUsername(@Nullable String username) {
        if (username == null || username.trim().isEmpty()) {
            prefs.edit().remove(KEY_USERNAME).apply();
        } else {
            prefs.edit().putString(KEY_USERNAME, username).apply();
        }
    }

    public void setPassword(@Nullable String password) {
        if (password == null || password.trim().isEmpty()) {
            prefs.edit().remove(KEY_PASSWORD).apply();
        } else {
            prefs.edit().putString(KEY_PASSWORD, password).apply();
        }
    }

    public void setTopicPrefix(@NonNull String prefix) {
        prefs.edit().putString(KEY_TOPIC_PREFIX, prefix).apply();
    }

    public void setQos(int qos) {
        if (qos < 0 || qos > 2) {
            throw new IllegalArgumentException("QoS must be 0, 1, or 2");
        }
        prefs.edit().putInt(KEY_QOS, qos).apply();
    }

    public void setRetain(boolean retain) {
        prefs.edit().putBoolean(KEY_RETAIN, retain).apply();
    }

    public void setUseWebSocket(boolean useWebSocket) {
        prefs.edit().putBoolean(KEY_USE_WEBSOCKET, useWebSocket).apply();
    }

    public void setClientId(@NonNull String clientId) {
        prefs.edit().putString(KEY_CLIENT_ID, clientId).apply();
    }

    /**
     * Builds the complete broker URI based on configuration.
     * Supports tcp://, mqtt://, ws://, wss://, mqtts:// schemes.
     */
    @NonNull
    public String getBrokerUri() {
        String url = getBrokerUrl();
        int port = getPort();
        
        // If URL already contains a scheme, use it as-is with port
        if (url.contains("://")) {
            // Extract scheme and host
            String[] parts = url.split("://", 2);
            String scheme = parts[0];
            String host = parts[1];
            
            // Remove any existing port from host
            if (host.contains(":")) {
                host = host.substring(0, host.indexOf(":"));
            }
            
            return scheme + "://" + host + ":" + port;
        }
        
        // No scheme provided, determine based on settings
        String scheme;
        if (isUseWebSocket()) {
            scheme = "ws";
        } else {
            scheme = "tcp";
        }
        
        return scheme + "://" + url + ":" + port;
    }

    /**
     * Gets the topic for glucose readings.
     */
    @NonNull
    public String getGlucoseTopic() {
        String prefix = getTopicPrefix();
        if (prefix.endsWith("/")) {
            return prefix + "glucose";
        }
        return prefix + "/glucose";
    }

    /**
     * Validates the current configuration.
     * @return null if valid, error message if invalid
     */
    @Nullable
    public String validate() {
        String url = getBrokerUrl();
        if (url == null || url.trim().isEmpty()) {
            return "Broker URL is required";
        }
        
        int port = getPort();
        if (port < 1 || port > 65535) {
            return "Port must be between 1 and 65535";
        }
        
        String prefix = getTopicPrefix();
        if (prefix == null || prefix.trim().isEmpty()) {
            return "Topic prefix is required";
        }
        
        // Validate topic prefix doesn't contain invalid characters
        if (prefix.contains("#") || prefix.contains("+")) {
            return "Topic prefix cannot contain wildcards (# or +)";
        }
        
        int qos = getQos();
        if (qos < 0 || qos > 2) {
            return "QoS must be 0, 1, or 2";
        }
        
        return null; // Valid
    }

    /**
     * Checks if authentication is configured.
     */
    public boolean hasAuthentication() {
        String username = getUsername();
        return username != null && !username.trim().isEmpty();
    }

    /**
     * Resets all settings to defaults.
     */
    public void resetToDefaults() {
        prefs.edit().clear().apply();
    }

    @Override
    public String toString() {
        return "MqttConfig{" +
                "brokerUri=" + getBrokerUri() +
                ", topic=" + getGlucoseTopic() +
                ", qos=" + getQos() +
                ", retain=" + isRetain() +
                ", hasAuth=" + hasAuthentication() +
                ", useWebSocket=" + isUseWebSocket() +
                '}';
    }
}

// Made with Bob
