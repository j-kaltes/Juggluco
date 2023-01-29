APPDIR=/home/jka/src/android/Glucodata
#SU=/data/local/tmp/mtk-su
#SU=/sbin/su
#SU=/su/bin/su
#ADB="adb -s RQ3006DWZL "
#ADB="adb -s  JQE6R18402000126 "
#ADB="adb -s  75bc9a40 "
ADB="adb -s  $1"
#ABI=armeabi-v7a
ABI=$2
SU=$3
btype=$4
ANVERSION=$5
SUBDIRS="share net curve nums settings LibAscon/src LibAscon/inc"
export OUTPUTDIR=$APPDIR/Common/build/mij/$ABI/$btype

	sh cmakemake.sh $btype $ABI $ANVERSION
if test ! -d $OUTPUTDIR
then
	sh cmakemake.sh $btype $ABI $ANVERSION
fi

cd $OUTPUTDIR

if make
then 
	$ADB shell rm -rf /data/local/tmp/glucose
	for fil in $SUBDIRS
	do
		$ADB shell mkdir -p /data/local/tmp/glucose/$fil
	done
	$ADB push $OUTPUTDIR/libg.so /data/local/tmp/glucose
	$ADB push $OUTPUTDIR/libnative.so /data/local/tmp/glucose
#	$ADB shell su -c "'cp /data/local/tmp/glucose/libg.so /data/app/none.none.glucose*/lib/arm64'"
#	echo $ADB shell $SU -c "'cp /data/local/tmp/glucose/libg.so /data/app/tk.glucodata-*/lib/arm64'"
	$ADB shell $SU -c sh /data/local/tmp/cplib.sh tk.glucodata.debug*
#	$ADB shell $SU -c "'cp /data/local/tmp/glucose/libg.so /data/app/tk.glucodata-*/lib/arm64'"
	echo done
	cd $OLDPWD
	$ADB push *.cpp *.h  /data/local/tmp/glucose
	for fil in $SUBDIRS
	do
		$ADB push $fil/*.cpp $fil/*.h $fil/*.c   /data/local/tmp/glucose/$fil
	done
fi
