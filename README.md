# Juggluco
Juggluco is an app that receives glucose values via Bluetooth from Freestyle Libre 0, 2 and 3 sensors. These glucose values are displayed in a graph to which other diabetes related numbers can be added. Juggluco can scan NovoPen® 6 and NovoPen Echo® Plus.
Juggluco can send glucose values to all kinds of smartwatches, see left menu→Watch→Help.
The phone version of Juggluco can talk out incoming glucose values, display it in a widget on the home screen and in a floating glucose above other apps.
Juggluco creates the usual statistics.
Other apps  can receive the data in Juggluco via glucose broadcasts and the web server in Juggluco. Juggluco can send data to Health Connect, Libreview and Nightscout. Data can also be exported to a file or send to another exemplar of Juggluco on an other phone, tablet, emulator or watch.
It has the option to set low and high glucose alarms and medication reminders.
<h4>Start</h4>
To use a Libre 2 sensor, simply scan it with Juggluco. 
To take over a Libre 3 sensor, you need to enter in Juggluco the same Libreview account as used when activating the sensor. A number received from Libreview is send to the sensor and needs to be the same as when the sensor was activated. Juggluco can’t take over Libre 3 sensors activated with Libre 3 reader. A Libreview account is not needed when the Libre 3 sensor is activated with Juggluco: simply enter an arbitrary number for the Libreview account number.
The first time it takes 2 to 10 minutes before Juggluco receives a glucose value via Bluetooth from the sensor. To prevent interference, force close apps and turn off devices previously used with the sensor. To keep Juggluco running in the background, allow background activity and turn off battery optimizations for Juggluco, under app info or battery in the device settings. Don’t hide Juggluco’s notification.
After scanning a sensor with Juggluco, Libre 2 sensors can only be used with Abbott’s Libre 2 app by scanning. Running them both in parallel can give connection problems, although two sensors on the same phone can receive in parallel glucose values from European Libre 2 sensors.
Libre 3 sensors can still be used with Abbott’s Libre 3 app after using it with Juggluco: stop Juggluco and scan the sensor with Abbott’s Libre 3 app agreeing that you stop the current sensor and start a new one. When going back to Juggluco you have to scan again.
WARNING: Glucose sensors are not always accurate. When a glucose reading disagrees with your feelings, use a blood glucose test strip.
<h1>WearOS</h1>
To make Juggluco for WearOS work, you first make the sensor work with Juggluco on the companion phone of the watch. Ensure that WIFI and Bluetooth are turned on on both phone and watch and Juggluco is installed on the watch and running. Then in Juggluco on the phone press left menu→WearOS config→Init watch app. Wait until Juggluco on the watch is receiving glucose values every minute from Juggluco on the phone. On WearOS 4 and lower you can use the watch face contained in Juggluco to display the glucose value. Under all version of WearOS, you can use a glucose complication contained in Juggluco. It can be added to watch faces that contain a small image complication slot.
To directly connect the sensor with the watch set left menu→Watch→WearOS config→”Direct sensor-watch connection”. Bluetooth has problems with going through water and your body contains a lot of water. Probably because of that, the connection between sensor and watch is better when you wear your watch on the same arm as the sensor.

## BUILD Juggluco
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


