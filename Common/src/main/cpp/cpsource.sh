#DEVICE=JQE6R18402000126	
DEVICE=1cf6c96

ADB="adb -s  $DEVICE "
$ADB shell rm /data/local/tmp/glucose
$ADB shell mkdir /data/local/tmp/glucose
$ADB push *.cpp *.h  /data/local/tmp/glucose
SUBDIRS="share net curve nums settings LibAscon/src LibAscon/inc net/libreview net/watchserver"
for fil in $SUBDIRS
do
	thedir=/data/local/tmp/glucose/$fil

	$ADB shell mkdir -p $thedir
	$ADB push $fil/*.cpp $fil/*.h $fil/*.c  $thedir 
done
