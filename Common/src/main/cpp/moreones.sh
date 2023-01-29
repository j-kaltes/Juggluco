#DIRS="/sdcard/libre2/E007-0M000DV8V60/2020-12-11-23:57:30 /sdcard/libre2/E007-0M000DV8V60/2020-12-12-03:18:17 /sdcard/libre2/E007-0M000DV8V60/2020-12-12-07:53:42 /sdcard/libre2/E007-0M000DV8V60/2020-12-12-08:02:59 /sdcard/libre2/E007-0M000DV8V60/2020-12-12-08:33:43"
DIRS=`cat testfiles`
for d in $DIRS
do
echo $d
/tmp/glucosex86_64/jnisubmain  -o $d
done

