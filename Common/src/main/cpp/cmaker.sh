#s/\/home\/jka\/src\/android\/Librefree/$APPDIR/g
APPDIR=/home/jka/src/android/Glucodata
cmake\
	-H$APPDIR/app/src/main/cpp\
	-DCMAKE_CXX_FLAGS=-std=c++20\
	-DCMAKE_FIND_ROOT_PATH=$APPDIR/app/.cxx/cmake/debug/arm64-v8a\
	-DCMAKE_BUILD_TYPE=Debug\
	-DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake\
	-DANDROID_ABI=arm64-v8a\
	-DANDROID_NDK=$ANDROID_NDK\
	-DANDROID_PLATFORM=android-24\
	-DCMAKE_ANDROID_ARCH_ABI=arm64-v8a\
	-DCMAKE_ANDROID_NDK=$ANDROID_NDK\
	-DCMAKE_EXPORT_COMPILE_COMMANDS=ON\
	-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=$APPDIR/app/build/intermediates/cmake/debug/obj/arm64-v8a\
	-DCMAKE_MAKE_PROGRAM=/home/jka/Android/Sdk/cmake/3.10.2.4988404/bin/ninja\
	-DCMAKE_SYSTEM_NAME=Android\
	-DCMAKE_SYSTEM_VERSION=19\
	-DCMAKE_VERBOSE_MAKEFILE=1\
	-B$APPDIR/app/.cxx/cmake/debug/arm64-v8a\
	-GNinja



