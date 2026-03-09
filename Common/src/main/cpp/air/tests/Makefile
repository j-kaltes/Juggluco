
#CXX=/home/jka/Android/Sdk/ndk/28.1.13356709/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android26-clang++
CXX=aarch64-linux-android26-clang++
#CXX=aarch64-linux-android21-clang++
#CXX=armv7a-linux-androideabi31-clang++
#CXX=/home/jka/Android/Sdk/ndk/25.2.9519653/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android26-clang++
CXXFLAGS=-DLOGSUB=1 -g
JDIR=../..
testrun: testrun.cpp  $(JDIR)/jnisub.cpp airfuncs.cpp Makefile logs.hpp ../air.hpp
	$(CXX) $(CXXFLAGS) -I. -I..  -I$(JDIR) -rdynamic -static-libstdc++ -lc++_static -DANDROID_STL=c++_static  -DDEXCOM=1 -DMAIN=1  -std=c++26   -Wno-vla-cxx-extension  testrun.cpp airfuncs.cpp $(JDIR)/jnisub.cpp  -o $@ -lc++_static -Wl,-rpath,'$$ORIGIN'

push:
	adb push testdata testdata.cpp testalg testrun  testalg.cpp testrun.cpp airfuncs.cpp $(JDIR)/jnisub.cpp $(JDIR)/inout.hpp $(JDIR)/share/hexstr.hpp logs.hpp $(JDIR)/jnisubin.hpp $(JDIR)/jnisub.hpp $(JDIR)/statedata.cpp $(JDIR)/subtrace.c $(JDIR)/subtrace.h $(JDIR)/streamdata.hpp /data/local/tmp/air

air: air.cpp
	g++ -std=c++26 -I. -I.. -I$(JDIR) air.cpp -o air
showsize: showsize.cpp
	g++ -std=c++26 -Itests -I.. showsize.cpp -o showsize

testalg: testalg.cpp  $(JDIR)/jnisub.cpp airfuncs.cpp Makefile logs.hpp ../air.hpp
	$(CXX) $(CXXFLAGS) -I. -I.. -I$(JDIR)  -rdynamic -static-libstdc++ -lc++_static -DANDROID_STL=c++_static  -DDEXCOM=1 -DMAIN=1  -std=c++26   -Wno-vla-cxx-extension  testalg.cpp airfuncs.cpp $(JDIR)/jnisub.cpp  -o $@ -lc++_static -Wl,-rpath,'$$ORIGIN'
testdata: testdata.cpp $(JDIR)/jnisub.cpp airfuncs.cpp Makefile logs.hpp ../air.hpp
	$(CXX) $(CXXFLAGS) -DDYNAMIC  -I. -I.. -I$(JDIR) -I$(JDIR)/LibAscon/inc -I$(JDIR)/share -rdynamic -static-libstdc++ -lc++_static -DANDROID_STL=c++_static  -DDEXCOM=1 -DMAIN=1  -std=c++26   -Wno-vla-cxx-extension  testdata.cpp airfuncs.cpp $(JDIR)/jnisub.cpp  -o $@ -lc++_static -Wl,-rpath,'$$ORIGIN'
testdatanew: testdata.cpp  $(JDIR)/jnisub.cpp airfuncs.cpp Makefile logs.hpp ../air.hpp
	clang++ $(CXXFLAGS) -L. -I. -I.. -I$(JDIR) -I$(JDIR)/LibAscon/inc -I$(JDIR)/share -DDEXCOM=1 -DMAIN=1  -std=c++26   -Wno-vla-cxx-extension  testdata.cpp airfuncs.cpp $(JDIR)/jnisub.cpp  -o $@  -lcaresens_air 
