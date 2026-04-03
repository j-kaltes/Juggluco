![value](valuemmolL.png)
# Juggluco
<h1>Libre 2/2+/3/3+, Sibionics GS1, Dexcom G7/1+, Accu-Chek, CareSensAir, Aidex X</h1>
<p style="line-height: 100%; margin-bottom: 0cm">The motivation to
write Juggluco was the need to use the sensor glucose values for
something more than knowing my current glucose level or getting a
global view the success of my glucose control.</p>
<p style="margin-bottom: 0cm; line-height: 100%">I wanted to use the
glucose curve to easily refer back to previous experiences and see
exactly how it went in relation to insulin doses, carbohydrate intake
and activities. A little isolated slowly generated curve in portrait
mode doesn’t allow for that.</p>
<p style="line-height: 100%; margin-bottom: 0cm">Juggluco is an app
that receives glucose values via Bluetooth from Freestyle Libre 2,
2+, 3 and 3+, Sibionics GS1Sb, Dexcom G7/ONE+, Accu-Chek SmartGuide,
CareSens Air and Aidex X sensors. In addition, Juggluco can scan
NovoPen®&nbsp;6 and NovoPen Echo®&nbsp;Plus and receive test
results via Bluetooth from Contour Next glucose meters. 
</p>
<p style="line-height: 100%; margin-bottom: 0cm">Juggluco can send
glucose values to all kinds of smartwatches, see left
menu→Watch→Help:
<a href="https://www.juggluco.nl/Jugglucohelp/watchinfo.html">https://www.juggluco.nl/Jugglucohelp/watchinfo.html</a>
</p>
<p style="line-height: 100%; margin-bottom: 0cm">The phone version of
Juggluco can talk out incoming glucose values, display it in a widget
on the home screen and in floating glucose above other apps. Juggluco
can display the usual glucose statistics. Other apps can receive data
from Juggluco via glucose broadcasts and the web server in Juggluco.
Juggluco can send data to Health Connect, Libreview and Nightscout.
Data can also be exported to a file or send to another exemplar of
Juggluco on another phone, tablet, emulator or watch. It has the
option to set low and high glucose alarms and medication reminders.</p>
<p style="line-height: 100%; margin-bottom: 0cm">&lt;h4&gt;Start&lt;/h4&gt;To
use a &lt;b&gt;Libre 2&lt;/b&gt; sensor, scan it with Juggluco. To
take over a &lt;b&gt;Libre 3&lt;/b&gt; sensor from Abbott's Libre
3-only app, you need to enter in left menu→Settings→Exchange
data→Libreview the same account as when activating the sensor and
press &lt;i&gt;Get account ID&lt;/i&gt;, to receive a number from
Libreview. When you now scan the sensor this number will be sent to
the sensor. No Libreview account is needed, when the Libre 3 sensor
is activated with Juggluco: enter an arbitrary number before scanning
the sensor. The first time it takes 2 to 10 minutes before Juggluco
receives a glucose value via Bluetooth from the sensor. To prevent
interference, force stop apps and turn off devices previously used
with the sensor. To keep Juggluco running in the background, allow
background activity and turn off battery optimizations for Juggluco.
Don’t hide Juggluco’s notification. 
</p>
<p style="line-height: 100%; margin-bottom: 0cm">After scanning a
Libre 2 sensor with Juggluco, Abbott’s Libre 2 app can only scan
the sensor (when two apps are on the same phone they can both receive
glucose values from European Libre 2 sensors, but this can give
connection problems.) Libre 3 sensors can still be used with Abbott’s
Libre 3 only app after using it with Juggluco: stop Juggluco and scan
the sensor with Abbott’s Libre 3 app, agreeing that you stop the
current sensor and start a new one. When going back to Juggluco you
have to scan again. 
</p>
<p style="line-height: 100%; margin-bottom: 0cm">Scan with left
menu→&lt;i&gt;Photo&lt;/i&gt; the data matrix on the applicator for
of &lt;b&gt;Dexcom G7/ONE+&lt;/b&gt;, the package of &lt;b&gt;Sibionics
GS1Sb&lt;/b&gt; and &lt;b&gt;Aidex X&lt;/b&gt; and the inner package
of &lt;b&gt;CareSens Air&lt;/b&gt;, and the blue cap of &lt;b&gt;Accu-Check
SmartGuide&lt;/b&gt; sensors. For the last enter the pin when asked.
Warning: CareSens Air sensors can switch device only once. See:
https://www.juggluco.nl/Juggluco/sensors</p>
<p style="line-height: 100%; margin-bottom: 0cm">&lt;h1&gt;Wear
OS&lt;/h1&gt;The phone version of Juggluco scans the sensor and sends
this data to the WearOS version of Juggluco. Bluetooth on both phone
and watch needs to be turned on. For fast transmission, also WIFI.
After initialization, you can switch on and off directly connecting
the sensor with the watch, with left menu→Watch→Wear OS
config→&lt;i&gt;Direct sensor-watch connection&lt;/i&gt;.
https://github.com/j-kaltes/Juggluco/wiki#wearos-watches collects
information about how well different watches work with Juggluco. 
</p>
<p style="line-height: 100%; margin-bottom: 0cm">Wear OS version
contains complications to display the glucose level on a watch face.</p>
<p style="line-height: 100%; margin-bottom: 0cm"><br/>

</p>
<p style="line-height: 100%; margin-bottom: 0cm">https://www.juggluco.nl/Jugglucohelp/introhelp.html</p>
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


