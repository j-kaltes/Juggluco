DIRS="arm64-v8a x86_64 x86 armeabi-v7a"
for d in $DIRS
do
	cp	jniLibs/$d/libcalibrate.so ../jniLibs/$d/libcalibrat2.so
done
