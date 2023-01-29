#CXXFLAGS=-std=c++20 -O3
#CXXFLAGS=-DANDROID_STL=c++_static -static-libstdc++ -std=c++20 -g -DMAIN -I./include 
CXXFLAGS=-std=c++20 -g -DMAIN -I./include 
LOADLIBES=-lpthread
#LOADLIBES=-lc++_static
CXX=clang++

#CXX=aarch64-linux-android26-clang++
#receive:receive.o mkdirs.o
all: backup sender
sendmagic.h:sendmagic 
	/home/jka/apps/bed/utils/bin2byte sendmagic sendmagic sendmagic.h uit

receivemagic.h:receivemagic 
	/home/jka/apps/bed/utils/bin2byte receivemagic receivemagic receivemagic.h uit


sender: sender.o  sendcommands.o netstuff.o
	$(CXX) $(CXXFLAGS) $^ -o $@  $(LOADLIBES)
backup: backup.o  getcommand.o netstuff.o
	$(CXX) $(CXXFLAGS) $^ -o $@  $(LOADLIBES)

testbackup: testbackup.o  getcommand.o
	$(CXX) $(CXXFLAGS) $^ -o $@  $(LOADLIBES)

netstuff:netstuff.o
	$(CXX) $(CXXFLAGS) $^ -o $@  $(LOADLIBES)
clean:
	-rm *.o backup sender
netstuff.i:netstuff.cpp
	$(CXX) -E $(CXXFLAGS) $^ -o $@  $(LOADLIBES)
