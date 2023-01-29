LDFLAGS=-s -static-libstdc++ -lc++_static
CXXFLAGS=-DANDROID_STL=c++_static -O3 -std=c++20 
CXX=aarch64-linux-android-g++
#LDFLAGS=
#CXXFLAGS=-O3 -std=c++20 
#CXXFLAGS=-O3 -std=c++20 
#XX=clang++

database:database.cpp database.h fooddata.h Makefile
	$(CXX) -s $(CXXFLAGS) -I.. -I. database.cpp  -o database $(LDFLAGS)

