uitdir=/n/ojka/src/android/Juggluco/libs/2.4.2

#infile='/n/ojka/Downloads/FreeStyle Libre 2 CA_v2.5.3_apkpure.com.apk'

infile='/n/ojka/Downloads/FreeStyle LibreLink DE_v2.4.2_apkpure.com.apk'


cd /n/ojka/src/android/libre2.5.3-CA/app/src/main/lib/
for dir in  *
do
echo $dir
mkdir -p $uitdir/$dir
java -cp /home/jka/prog/java unzip  "$infile" lib/$dir/libDataProcessing.so $uitdir/$dir/libcalibrate.so
done

