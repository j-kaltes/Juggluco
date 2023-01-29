
CXX=g++-10
CXXFLAGS=-std=c++20 -I./share -static-libstdc++   
export APP_ABI=x86_64
#export APP_ABI=armeabi-v7a
ifeq ($(APP_ABI),armeabi-v7a)
	ACC=armv7a-linux-androideabi29-clang
else
	ifeq ($(APP_ABI),arm64-v8a)
		GCCARCH=aarch64
	else
		ifeq ($(APP_ABI),x86)
			GCCARCH=i686
		else
			GCCARCH=$(APP_ABI)
		endif
	endif
ACC=$(GCCARCH)-linux-android24-clang
endif
flags:
	echo $(CXXFLAGS)
nfcdata: nfcdata.cpp history.o logs.o nfcdata.h history.h
	$(CXX) -g -DMAIN $(CXXFLAGS) history.o logs.o nfcdata.cpp -o $@ 
#tags: DataProcessingNative.h abbott.cpp abbottclass.h abbottdec.h history.h javaclass.h localhost_librefree_re_flashglucose_sensorabstractionservice_dataprocessing_DataProcessingNative.h native-lib.cpp nfcdata.cpp nfcdata.h
#	ctags  $^

#export APP_ABI=arm64-v8a
#export APP_ABI=x86

ACXX=$(ACC)++

test:
	echo $(ACXX)

#ACXX=x86_64-linux-android24-clang++
#ACC=x86_64-linux-android24-clang
#ACXX=i686-linux-android26-clang++
#ACXX=aarch64-linux-android26-clang++
#ACC=aarch64-linux-android26-clang
#AACXX=aarch64-linux-android26-clang++
#AACC=aarch64-linux-android26-clang
#ACC=x86_64-linux-android26-clang
#ACXX=g++
#ACC=gcc
#LIBRARYPATH=/data/local/tmp/lib/$(APP_ABI)
#LIBRARYPATH=/o/home/jka/src/android/librelibs/2.4.2/$(APP_ABI)
#LIBRARYPATH=/o/home/jka/src/android/librelibs/2.4.0/$(APP_ABI)
LIBRARYPATH=/o/home/jka/src/android/librelibs/2.5.2/$(APP_ABI)

#ACLDFLAGS+= -O0  -g  -fno-exceptions   -rdynamic 
#ACLDFLAGS+= -O0  -g  -fno-exceptions   -Wl,--export-dynamic -Wl,-Bdynamic
#ACXXFLAGS=-std=c++2a -O0   -gdwarf-4 -g3   -I. -fno-exceptions -DLIBRARYDIR='"$(LIBRARYPATH)"' 
#ACFLAGS=-O0     -gdwarf-4 -g3  -fno-exceptions
ACLDFLAGS+= -O3  -g  -fno-exceptions   -Wl,--export-dynamic -Wl,-Bdynamic
ACXXFLAGS=-static-libstdc++  -std=c++20 -I./share -O3   -gdwarf-4 -g3   -I. -fno-exceptions -DLIBRARYDIR='"$(LIBRARYPATH)"' 
ACFLAGS=-O3     -gdwarf-4 -g3  -fno-exceptions

DYNLINK=yes
ifeq ($(DYNLINK),yes)
#ACLDFLAGS+=-rdynamic 
ACLDFLAGS+=-Wl,--export-dynamic -Wl,-Bdynamic
ACXXFLAGS+=-DDYNLINK
else
ACLDFLAGS+=-Xlinker -rpath=$(LIBRARYPATH)
endif

#ACXXFLAGS=-std=c++2a   -g3 
jnisubmain: jnisubmain.o jnisub.o nfcdata.o  serial.o  logs.o hooks.o  statedata.o timestr.o savehistory.o
#	$(ACXX) $(ACXXFLAGS)  $^ -o $@ -Llib/x86 -lDataProcessing
#	$(ACXX)  $(ACXXFLAGS)  $^ -o $@ -L/system/lib64 -lc -lc++ -lc -lDataProcessing -lc -lc++ -lc 
ifeq ($(DYNLINK),yes)
	$(ACXX)  $(ACLDFLAGS)   $^ -o $@   -lc -lc++ -lc 
else
	$(ACXX)  $(ACLDFLAGS)   $^ -o $@  -Llib/$(APP_ABI)  -lc -lc++ -lc -lDataProcessing
endif
#	$(ACXX)  $(ACXXFLAGS)  $^ -o $@ -Llib/x86_64 -lc -lc++ -lc -lDataProcessing
#	$(ACXX)  $(ACXXFLAGS)  $^ -o $@  -Llib/arm64-v8a -lDataProcessing

	#$(ACXX) -rdynamic $(ACXXFLAGS)  $^ -o $@ -ldl
jnisubmain.o: jnisubmain.cpp Makefile
	$(ACXX) $(ACXXFLAGS)  -c  jnisubmain.cpp 
jnisub.o: jnisub.cpp Makefile
	$(ACXX) $(ACXXFLAGS) -c jnisub.cpp 
nfcdata.o: nfcdata.cpp
	$(ACXX) $(ACXXFLAGS) -c nfcdata.cpp 

serial: serial.cpp
	$(ACXX) -DMAIN $(ACXXFLAGS)   $^ -o $@
clean:
	rm jnisubmain jnisubmain.o jnisub.o nfcdata.o  serial.o  logs.o hooks.o  statedata.o timestr.o savehistory.o
%:%.cpp
	$(ACXX) $(ACXXFLAGS)   $^ -o $@
%.o:%.cpp
	$(ACXX) -c $(ACXXFLAGS)   $^ -o $@
%.o:%.c
	$(ACC) -c $(ACFLAGS)   $^ -o $@


#s/ / \/system\/lib64\//g
libs.tgz: /system/lib64/libc++_shared.so /system/lib64/libm.so /system/lib64/libdl.so /system/lib64/libc.so /system/lib64/liblog.so  /system/bin/linker64 /system/lib64/libDataProcessing.so /system/lib64/libstdc++.so
	tar -zcf libs.tgz $^

#link86: linktest.o  logs.o
link86: linktest.o hooks.o logs.o
#	$(ACXX) $(ACXXFLAGS)     $^ -o $@ -L4   -llog -lm
#	$(ACXX) $(ACXXFLAGS)     $^ -o $@ -L/system/lib64  -lDataProcessing -llog -lm
#	$(ACXX) $(ACXXFLAGS)     $^ -o $@ -Llib/x86_64  -lDataProcessing -llog -lm
#	$(ACXX)  $(ACXXFLAGS)  $^ -o $@ -Llib/x86_64 -lc -lc++ -lc -lDataProcessing
	$(ACXX) $(ACXXFLAGS)    $^ -o $@  -Llib/$(APP_ABI)  -lc -lc++ -lc -lDataProcessing
	#$(ACXX) $(ACXXFLAGS)  -nostartfiles    $^ -o $@ -L/system/lib64  -lDataProcessing -llog -lm

linktest: linktest.o hooks.o logs.o
	$(ACXX) $(ACXXFLAGS)  $(ACLDFLAGS)  $^ -o $@  -Llib/$(APP_ABI)  -lc -lc++ -lc -lDataProcessing
#	$(ACXX) $(ACXXFLAGS)    $^ -o $@  -Llib/arm64-v8a -lDataProcessing -llog -lm


statedata: statedata.cpp timestr.cpp logs.cpp

#	g++ -std=c++20 -g3 $^ -o $@

.PHONY: tags

tags:
	ctags -R --c++-kinds=+p --fields=+iaS --extra=+q .
