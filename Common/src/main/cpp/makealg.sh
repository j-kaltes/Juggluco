APPDIR=/home/jka/src/android/Glucodata
#SU=/data/local/tmp/mtk-su
#SU=/sbin/su
SU=/su/bin/su
#ADB="adb -s RQ3006DWZL "
#ADB="adb -s  JQE6R18402000126 "
ADB="adb -s  75bc9a40 "
export OUTPUTDIR=$APPDIR/app/build/mij/debug

if test ! -d $OUTPUTDIR
then
	sh cmakealg.sh armeabi-v7a
fi

cd $OUTPUTDIR

if make
then 
	$ADB shell mkdir -p /data/local/tmp/glucose/share
	$ADB shell mkdir -p /data/local/tmp/glucose/net
	$ADB shell mkdir -p /data/local/tmp/glucose/curve
	$ADB push $OUTPUTDIR/libg.so /data/local/tmp/glucose
#	$ADB shell su -c "'cp /data/local/tmp/glucose/libg.so /data/app/none.none.glucose*/lib/arm64'"
#	echo $ADB shell $SU -c "'cp /data/local/tmp/glucose/libg.so /data/app/tk.glucodata-*/lib/arm64'"
	$ADB shell $SU -c sh /data/local/tmp/cplib.sh
#	$ADB shell $SU -c "'cp /data/local/tmp/glucose/libg.so /data/app/tk.glucodata-*/lib/arm64'"
	echo done
	cd $OLDPWD
	$ADB push *.cpp *.h  /data/local/tmp/glucose
	$ADB push net/*.cpp net/*.h  /data/local/tmp/glucose/net
	$ADB push curve/*.cpp curve/*.h  /data/local/tmp/glucose/curve
	$ADB push share/*.cpp share/*.h  /data/local/tmp/glucose/share
fi
