# Fork and Deploy Guide - Juggluco with MQTT

This guide walks you through forking the Juggluco repository and deploying the MQTT-enabled version.

## Step 1: Fork the Repository on GitHub

### Option A: Using GitHub Web Interface

1. **Navigate to the original Juggluco repository:**
   ```
   https://github.com/j-kaltes/Juggluco
   ```

2. **Click the "Fork" button** in the top-right corner

3. **Configure your fork:**
   - Choose your GitHub account as the owner
   - Keep the repository name as "Juggluco" or rename it
   - Add a description (optional): "Juggluco with MQTT integration"
   - Click "Create fork"

4. **Your fork is now available at:**
   ```
   https://github.com/YOUR_USERNAME/Juggluco
   ```

### Option B: Using GitHub CLI

```bash
# Install GitHub CLI if not already installed
# https://cli.github.com/

# Fork the repository
gh repo fork j-kaltes/Juggluco --clone=false

# Clone your fork
gh repo clone YOUR_USERNAME/Juggluco
cd Juggluco
```

## Step 2: Apply MQTT Integration Changes

### Method 1: Manual Application (Recommended)

Since you have the MQTT integration code ready, apply the changes to your fork:

1. **Clone your fork:**
   ```bash
   git clone https://github.com/YOUR_USERNAME/Juggluco.git
   cd Juggluco
   ```

2. **Create a feature branch:**
   ```bash
   git checkout -b feature/mqtt-integration
   ```

3. **Copy the MQTT integration files** from your local Juggluco directory:
   
   **New files to add:**
   ```bash
   # MQTT package
   Common/src/main/java/tk/glucodata/mqtt/MqttConfig.java
   Common/src/main/java/tk/glucodata/mqtt/MqttPublisher.java
   
   # Documentation
   MQTT_INTEGRATION.md
   FORK_AND_DEPLOY.md
   ```

4. **Modified files to update:**
   ```bash
   Common/build.gradle
   Common/src/main/AndroidManifest.xml
   Common/src/main/java/tk/glucodata/Natives.java
   Common/src/main/java/tk/glucodata/SuperGattCallback.java
   Common/src/main/java/tk/glucodata/settings/Broadcasts.java
   Common/src/main/java/tk/glucodata/settings/Settings.java
   ```

5. **Commit your changes:**
   ```bash
   git add .
   git commit -m "Add MQTT integration for glucose data publishing"
   ```

6. **Push to your fork:**
   ```bash
   git push origin feature/mqtt-integration
   ```

### Method 2: Using Patch File

If you have a patch file:

```bash
git apply mqtt-integration.patch
git add .
git commit -m "Add MQTT integration"
git push origin feature/mqtt-integration
```

## Step 3: Build the APK

### Prerequisites

- **Java Development Kit (JDK)** 17 or higher
- **Android SDK** with API level 35
- **Android NDK** version 29.0.14206865
- **Git**

### Build Steps

1. **Navigate to your cloned repository:**
   ```bash
   cd Juggluco
   ```

2. **Make gradlew executable (Linux/Mac):**
   ```bash
   chmod +x gradlew
   ```

3. **Build the release APK:**
   ```bash
   ./gradlew assembleMobileLibre3SiDexNogoogleRelease
   ```
   
   Or for other variants:
   ```bash
   # For Google Play version
   ./gradlew assembleMobileLibre3SiDexGoogleRelease
   
   # For Libre 2 version
   ./gradlew assembleMobileLibreOldSiNodexNogoogleRelease
   ```

4. **Find your APK:**
   ```
   Common/build/outputs/apk/mobileLibre3SiDexNogoogle/release/
   ```

### Build Variants

Juggluco has multiple build variants. Choose based on your needs:

| Variant | Description |
|---------|-------------|
| `mobileLibre3SiDexNogoogle` | Libre 3 + SiBionics + Dexcom, no Google services |
| `mobileLibre3SiDexGoogle` | Libre 3 + SiBionics + Dexcom, with Google services |
| `mobileLibreOldSiNodexNogoogle` | Libre 2 + SiBionics, no Dexcom, no Google |

## Step 4: Install on Your Device

### Method 1: Direct Installation

1. **Enable "Install from Unknown Sources"** on your Android device
2. **Transfer the APK** to your device
3. **Tap the APK** to install

### Method 2: Using ADB

```bash
adb install Common/build/outputs/apk/mobileLibre3SiDexNogoogle/release/Common-mobileLibre3SiDexNogoogle-release.apk
```

## Step 5: Configure MQTT

1. **Open Juggluco**
2. **Go to Settings → Left Menu → Connections**
3. **Enable "MQTT Publishing"**
4. **Tap "MQTT Config"**
5. **Configure your broker settings:**
   - Broker URL: `tcp://your-broker-ip`
   - Port: `1883`
   - Username/Password (if required)
   - Topic Prefix: `juggluco`
6. **Tap "Test Connection"**
7. **Tap "Save"**

See [MQTT_INTEGRATION.md](MQTT_INTEGRATION.md) for detailed configuration instructions.

## Step 6: Verify MQTT Publishing

### Using Mosquitto Client

```bash
# Subscribe to glucose readings
mosquitto_sub -h YOUR_BROKER_IP -t "juggluco/glucose" -v

# You should see JSON messages like:
# juggluco/glucose {"glucose":120.5,"unit":"mg/dL","timestamp":1708723200000,...}
```

### Using MQTT Explorer

1. Download [MQTT Explorer](http://mqtt-explorer.com/)
2. Connect to your broker
3. Subscribe to `juggluco/#`
4. Watch for glucose readings

## Troubleshooting

### Build Issues

**Problem:** `SDK location not found`
```bash
# Create local.properties file
echo "sdk.dir=/path/to/Android/Sdk" > local.properties
```

**Problem:** `NDK not found`
```bash
# Install NDK via Android Studio or sdkmanager
sdkmanager --install "ndk;29.0.14206865"
```

**Problem:** `Gradle sync failed`
```bash
# Clean and rebuild
./gradlew clean
./gradlew assembleMobileLibre3SiDexNogoogleRelease
```

### Runtime Issues

**Problem:** MQTT not connecting
- Check broker IP and port
- Verify network connectivity
- Check firewall settings
- Review Juggluco logs: `adb logcat | grep MqttPublisher`

**Problem:** No messages published
- Ensure MQTT Publishing is enabled
- Check if glucose readings are being received
- Verify broker is receiving connections

## Keeping Your Fork Updated

### Sync with Upstream

```bash
# Add upstream remote (one time)
git remote add upstream https://github.com/j-kaltes/Juggluco.git

# Fetch upstream changes
git fetch upstream

# Merge upstream changes into your branch
git checkout main
git merge upstream/main

# Rebase your feature branch
git checkout feature/mqtt-integration
git rebase main

# Push updates
git push origin feature/mqtt-integration --force-with-lease
```

## Contributing Back

If you want to contribute the MQTT integration back to the original Juggluco project:

1. **Ensure your code is clean and well-tested**
2. **Create a Pull Request** on GitHub:
   - Go to your fork on GitHub
   - Click "Contribute" → "Open pull request"
   - Describe your changes
   - Submit the PR

3. **Wait for review** from the maintainer

## Native Code Implementation (TODO)

The current implementation uses Java-side storage for the MQTT enabled flag. For full integration, you'll need to add native code support:

### Files to modify:

1. **Common/src/main/cpp/config.h** or similar
   - Add MQTT enabled flag storage

2. **Common/src/main/cpp/jnisub2.h** or similar
   - Add JNI methods:
     ```c
     JNIEXPORT jboolean JNICALL Java_tk_glucodata_Natives_getMqttEnabled(JNIEnv *env, jclass clazz);
     JNIEXPORT void JNICALL Java_tk_glucodata_Natives_setMqttEnabled(JNIEnv *env, jclass clazz, jboolean enabled);
     ```

3. **Implement the native methods** following the pattern of other broadcast flags (xbroadcast, Jugglucobroadcast)

For now, the MQTT enabled state is managed through SharedPreferences in MqttConfig.java, which works but doesn't persist across app reinstalls like native storage would.

## License

This fork maintains the original Juggluco license (GPL v3). See LICENSE.txt for details.

## Support

- **MQTT Integration Issues**: Open an issue on your fork
- **Juggluco Issues**: Refer to the [original repository](https://github.com/j-kaltes/Juggluco)
- **MQTT Documentation**: See [MQTT_INTEGRATION.md](MQTT_INTEGRATION.md)

## Resources

- [Juggluco Original Repository](https://github.com/j-kaltes/Juggluco)
- [Eclipse Paho MQTT](https://www.eclipse.org/paho/)
- [Mosquitto MQTT Broker](https://mosquitto.org/)
- [MQTT Protocol](https://mqtt.org/)
- [Home Assistant MQTT Integration](https://www.home-assistant.io/integrations/mqtt/)

## Quick Reference

### Build Commands
```bash
# Clean build
./gradlew clean

# Build release APK
./gradlew assembleMobileLibre3SiDexNogoogleRelease

# Build debug APK
./gradlew assembleMobileLibre3SiDexNogoogleDebug

# Install on connected device
./gradlew installMobileLibre3SiDexNogoogleDebug
```

### Git Commands
```bash
# Create feature branch
git checkout -b feature/mqtt-integration

# Commit changes
git add .
git commit -m "Add MQTT integration"

# Push to fork
git push origin feature/mqtt-integration

# Sync with upstream
git fetch upstream
git merge upstream/main
```

### MQTT Testing
```bash
# Subscribe to messages
mosquitto_sub -h localhost -t "juggluco/glucose" -v

# Publish test message
mosquitto_pub -h localhost -t "juggluco/glucose" -m '{"glucose":120}'

# Check broker status
mosquitto_sub -h localhost -t '$SYS/#' -v
```

---

**Happy coding! 🚀**