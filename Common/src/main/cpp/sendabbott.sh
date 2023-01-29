cd /home/jka/src/android/Librefree/app/.cxx/cmake/debug/arm64-v8a
ninja  DataProcessingNL  && adb -s RQ3006YK2M push /home/jka/src/android/Librefree/app/build/intermediates/cmake/debug/obj/arm64-v8a/* /sdcard/libre/libs&& (adb -s RQ3006YK2M shell su -c sh -x /sdcard/libre/hcopy.sh)

