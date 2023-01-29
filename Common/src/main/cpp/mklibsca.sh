uitdir=/n/ojka/src/android/libre2.5.3-CA/app/src/main/uitlib/
cd /n/ojka/src/android/libre2.5.3-CA/app/src/main/lib/
for dir in  *
do
echo $dir
mkdir -p $uitdir/$dir
java -cp /home/jka/prog/java unzip '/n/ojka/Downloads/FreeStyle Libre 2 CA_v2.5.3_apkpure.com.apk' lib/$dir/libDataProcessing.so $uitdir/$dir/libcalibrate.so
done

