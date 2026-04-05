# MQTT Integration for Juggluco

This document describes the MQTT integration feature added to Juggluco, which allows real-time publishing of glucose readings to an MQTT broker such as Mosquitto.

## Overview

The MQTT integration enables Juggluco to publish glucose readings to any MQTT broker, making it easy to integrate with home automation systems (Home Assistant, Node-RED, etc.) and other monitoring solutions.

## Features

- ✅ Real-time glucose data publishing
- ✅ Support for TCP and WebSocket protocols
- ✅ TLS/SSL encryption support (mqtts:// and wss://)
- ✅ Configurable QoS levels (0, 1, 2)
- ✅ Message retention option
- ✅ Automatic reconnection with exponential backoff
- ✅ Message queuing during disconnection
- ✅ Username/password authentication
- ✅ Configurable topic structure
- ✅ JSON message format

## Installation

### Prerequisites

1. An MQTT broker (e.g., Mosquitto, HiveMQ, EMQX)
2. Network connectivity between your Android device and the MQTT broker

### Building from Source

1. Fork the Juggluco repository on GitHub
2. Clone your fork:
   ```bash
   git clone https://github.com/YOUR_USERNAME/Juggluco.git
   cd Juggluco
   ```

3. The MQTT integration is already included in this fork with the following changes:
   - Eclipse Paho MQTT library dependencies added to `Common/build.gradle`
   - MQTT classes in `Common/src/main/java/tk/glucodata/mqtt/`
   - Integration into glucose broadcast system
   - Settings UI for configuration

4. Build the APK:
   ```bash
   ./gradlew assembleRelease
   ```

## Configuration

### 1. Enable MQTT Publishing

1. Open Juggluco
2. Go to **Settings** → **Left Menu** → **Connections**
3. Check the **MQTT Publishing** checkbox
4. Tap **MQTT Config** to configure the broker

### 2. Configure MQTT Broker

In the MQTT Configuration screen, set the following:

#### Broker URL
- **TCP**: `tcp://broker.example.com` or `mqtt://broker.example.com`
- **TLS**: `mqtts://broker.example.com`
- **WebSocket**: `ws://broker.example.com`
- **WebSocket Secure**: `wss://broker.example.com`

Examples:
- Local Mosquitto: `tcp://192.168.1.100`
- Cloud broker: `mqtts://mqtt.example.com`
- WebSocket: `ws://192.168.1.100`

#### Port
- **TCP**: 1883 (default)
- **TLS**: 8883 (default)
- **WebSocket**: 9001 (default)
- **WebSocket Secure**: 443 or 8084

#### Authentication (Optional)
- **Username**: Your MQTT username
- **Password**: Your MQTT password

#### Topic Prefix
- Default: `juggluco`
- Glucose readings will be published to: `{prefix}/glucose`
- Example: `juggluco/glucose`

#### QoS (Quality of Service)
- **QoS 0**: At most once (fire and forget)
- **QoS 1**: At least once (acknowledged delivery) - **Recommended**
- **QoS 2**: Exactly once (assured delivery)

#### Options
- **Use WebSocket**: Enable for WebSocket protocol (ws:// or wss://)
- **Retain Messages**: Keep last message on broker for new subscribers

### 3. Test Connection

1. Tap **Test Connection** to verify your settings
2. Check the status indicator:
   - 🟢 **Connected**: Successfully connected to broker
   - 🔴 **Disconnected**: Not connected
   - 🔴 **Error**: Connection failed (check settings)

3. Tap **Save** to save your configuration

## Message Format

Glucose readings are published as JSON messages:

```json
{
  "glucose": 120.5,
  "unit": "mg/dL",
  "timestamp": 1708723200000,
  "rate": 0.5,
  "rateUnit": "mg/dL/min",
  "alarm": 0,
  "sensorSerial": "ABC123",
  "sensorStartTime": 1708636800000,
  "sensorGen": 3
}
```

### Field Descriptions

| Field | Type | Description |
|-------|------|-------------|
| `glucose` | number | Current glucose value |
| `unit` | string | Unit of measurement ("mg/dL" or "mmol/L") |
| `timestamp` | number | Unix timestamp in milliseconds |
| `rate` | number | Rate of change |
| `rateUnit` | string | Unit for rate of change |
| `alarm` | number | Alarm status code |
| `sensorSerial` | string | Sensor serial number |
| `sensorStartTime` | number | Sensor start time (Unix timestamp) |
| `sensorGen` | number | Sensor generation/type |

## Integration Examples

### Home Assistant

Add to your `configuration.yaml`:

```yaml
mqtt:
  sensor:
    - name: "Blood Glucose"
      state_topic: "juggluco/glucose"
      unit_of_measurement: "mg/dL"
      value_template: "{{ value_json.glucose }}"
      json_attributes_topic: "juggluco/glucose"
      json_attributes_template: "{{ value_json | tojson }}"
      icon: mdi:diabetes
```

### Node-RED

Use an MQTT In node:
- **Server**: Your MQTT broker
- **Topic**: `juggluco/glucose`
- **Output**: Auto-detect (JSON object)

Then process with a function node:
```javascript
msg.payload = {
    glucose: msg.payload.glucose,
    timestamp: new Date(msg.payload.timestamp),
    trend: msg.payload.rate > 0 ? "rising" : "falling"
};
return msg;
```

### Mosquitto Client (Testing)

Subscribe to messages:
```bash
mosquitto_sub -h localhost -t "juggluco/glucose" -v
```

## Troubleshooting

### Connection Issues

1. **Cannot connect to broker**
   - Verify broker URL and port
   - Check network connectivity
   - Ensure broker is running
   - Check firewall settings

2. **Authentication failed**
   - Verify username and password
   - Check broker ACL (Access Control List)

3. **TLS/SSL errors**
   - Ensure broker certificate is valid
   - Use correct port (usually 8883 for mqtts://)

### Message Issues

1. **No messages received**
   - Check if MQTT Publishing is enabled
   - Verify topic subscription matches configuration
   - Check QoS settings
   - Review broker logs

2. **Messages delayed**
   - Check network latency
   - Consider using QoS 0 for faster delivery
   - Verify broker performance

### Debugging

Enable logging in Juggluco:
1. Go to Settings → Left Menu → About
2. Enable debug logging
3. Check logcat for MQTT-related messages:
   ```bash
   adb logcat | grep MqttPublisher
   ```

## Security Considerations

1. **Use TLS/SSL** for production deployments (mqtts:// or wss://)
2. **Enable authentication** on your MQTT broker
3. **Use strong passwords**
4. **Restrict broker access** with ACLs
5. **Keep broker software updated**
6. **Use VPN** when accessing broker over internet

## Performance

- **Message size**: ~200-300 bytes per glucose reading
- **Frequency**: Typically every 1-5 minutes (depends on sensor)
- **Bandwidth**: Minimal (~1-2 KB/hour)
- **Battery impact**: Negligible (MQTT is very efficient)

## Supported MQTT Brokers

Tested with:
- ✅ Eclipse Mosquitto
- ✅ HiveMQ
- ✅ EMQX
- ✅ AWS IoT Core
- ✅ Azure IoT Hub
- ✅ CloudMQTT

Should work with any MQTT 3.1.1 compliant broker.

## Advanced Configuration

### Custom Topics

To use a different topic structure, modify the topic prefix:
- Single topic: `home/health/glucose`
- Per-device: `devices/phone1/glucose`
- Per-user: `users/john/glucose`

### Multiple Brokers

Currently, only one broker can be configured at a time. To publish to multiple brokers, use MQTT bridge functionality on your primary broker.

### Message Retention

Enable "Retain Messages" to ensure new subscribers immediately receive the last glucose reading.

## FAQ

**Q: Does MQTT work in the background?**  
A: Yes, MQTT publishing works even when Juggluco is in the background.

**Q: What happens if the broker is offline?**  
A: Messages are queued (up to 100) and sent when connection is restored.

**Q: Can I use a cloud MQTT broker?**  
A: Yes, any accessible MQTT broker works (local or cloud).

**Q: Does this affect battery life?**  
A: Impact is minimal. MQTT is designed for low-power devices.

**Q: Can I disable MQTT temporarily?**  
A: Yes, uncheck "MQTT Publishing" in Settings → Connections.

## Support

For issues or questions:
1. Check this documentation
2. Review Juggluco logs
3. Test with mosquitto_sub
4. Open an issue on GitHub

## License

This MQTT integration follows the same license as Juggluco (GPL v3).

## Credits

- MQTT integration by [Your Name]
- Based on Juggluco by Jaap Korthals Altes
- Uses Eclipse Paho MQTT Android library

## Version History

- **v1.0** (2024): Initial MQTT integration
  - TCP and WebSocket support
  - TLS/SSL encryption
  - Configurable QoS and retention
  - Automatic reconnection
  - JSON message format