![Android Build Analyzer](buildanalyzer.png)
As you see Juggluco consist according to Github for 1.2% out of Kotlin and according to Android Studio Build Analyzer tasks related to 
Kotlin took 43.8% of the time to make the Debug version of Juggluco.
# Juggluco
An Android app that displays glucose values it receives via Bluetooth from Freestyle Libre 2 and 3 sensors.

Abbott's Freestyle Libre 2 and 3 sensors transmit every minute the perceived glucose value to the connected reader or smartphone. Juggluco displays this value on the smartphone in addition to low and high glucose alarms. Also with Libre 2 sensors you can know your current glucose value without scanning. Newer versions of Juggluco can also be used with Freestyle Libre 3 sensors, but all further remarks relate to Libre 2. See https://www.juggluco.nl/libre3 for more information about the use of Libre 3.  
After using a sensor with Juggluco, you can still scan the sensor with Librelink or Freestyle Reader. Freestyle Reader's Bluetooth connection will no longer work with this sensor and if Bluetooth of Freestyle Reader isn't turned off, it will interfere with the Bluetooth connection of Juggluco with the sensors. Librelink can still continue to work with European Libre 2 sensors, if Juggluco runs on the same phone and already has a connection with the sensor. Librelink can't get a connection on it's own and it can result in connection loss of both apps that is only solved by killing Librelink and thereafter turning Bluetooth off and on and only restarting Librelink again after Juggluco receives again glucose from the sensor. So you shouldn't run them in parallel, when you want to rely on glucose alarms. Libre 3 sensors and US Libre 2 sensors can't be used in parallel.
In addition to the Bluetooth stream of glucose values, Juggluco displays the result of scanning: the current value and past 8 hours history out of the memory of the sensor. All in one detailed graph.  
You can also add your amounts of insulin, carbohydrate and activity to the graph.  
From the Bluetooth stream data summary statistics are generated: time in range, estimated A1c and, if enough data is gathered, a summary graph.  
Juggluco can send glucose values to smartwatches in six ways:  
- by creating an Android notification that is redirected to some smartwatches (Notify);  
- via watch app Kerfstok running on some Garmin watches;  
- via the webserver in Juggluco that xDrip and Nightscout watch apps can access (left menu->Watch->Web server);  
- via glucose broadcasts to other Android apps that send the glucose values again to connected watches (for example G-Watch);
- Juggluco for WearOS;
- by sending glucose values to Watchdrip's (MiBand/Amazfit/ZeppOS).

Juggluco can use (but not start) **US** Freestyle Libre 2 sensors.

## WearOS

A variant of Juggluco now runs on WearOS. It contains a watch face that shows the minutely glucose value, besides the time and four complications.  
Every sensor has to be initialized by the connected Android phone running Juggluco.  
Juggluco for Wear OS can operate two ways:  
- The watch receives the glucose values from a smartphone connected to the sensor;  
- The watch is directly connected with the sensor and sends the glucose values to the phone.  

A direct connection between Watch 4 and sensor is unusable with nearly half of the Freestyle Libre 2 sensors and a fifth of the Freestyle Libre 3 sensors. If, when directly connected with the watch, a sensor has too many connection errors, you can connect the watch with the sensor via the smartphone.  
For more information: https://www.juggluco.nl/JugglucoWearOS

## BUILD Juggluco
Building seems not to work under Windows. In Common/build.gradle

``
cppFlags  " -DAPPID=\\\"$applicationId\\\" "
``

should result in APPID being defined with the Application ID surrounded by quotes (""). This happens under Linux, but not Windows.
Gradle and cmake should work independent of operation system, so it is their fault. And why do you use Microsoft Windows anyway?

The following files need to be added to build Juggluco and can be found by unzipping an Arm/Arm64/x86/x86_64 Juggluco apk from
https://www.juggluco.nl/Juggluco/download.html

libcalibrat2.so and libcalibrate.so in lib/* of the APK should be put in the corresponding directories (e.g. the libraries from armeabi-v7a of the apk should be put in armeabi-v7) in:

./Common/src/main/jniLibs/x86_64/    
./Common/src/main/jniLibs/armeabi-v7a/   
./Common/src/main/jniLibs/x86/   
./Common/src/main/jniLibs/arm64-v8a/   
   
libcrl_dp.so  liblibre3extension.so  and libinit.so  in the corresponding directories of:   
./Common/src/libre3/jniLibs/x86_64/   
./Common/src/libre3/jniLibs/armeabi-v7a/   
./Common/src/libre3/jniLibs/x86/   
./Common/src/libre3/jniLibs/arm64-v8a/   

libnative-algorithm-jni-v113B.so  libnative-encrypy-decrypt-v110.so  libnative-struct2json.so libnative-algorithm-v1_1_3_B.so   libnative-sensitivity-v110.so in   
./Common/src/mobileSi/jniLibs/armeabi-v7a/   
./Common/src/mobileSi/jniLibs/arm64-v8a/


