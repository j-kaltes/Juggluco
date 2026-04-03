#CC:=aarch64-linux-android-gcc
#CXX:=aarch64-linux-android-g++ 
CC=clang
CCXX=clang++
CFLAGS:=-O3  -std=c99 -Wall -Wextra -D_ISOC99_SOURCE -MMD -I./ 
CXXFLAGS:=-O3  -std=c++20 -Wall -Wextra -D_ISOC99_SOURCE -MMD -I./ -I../share

LINK.o = $(CXX) $(LDFLAGS)

LOADLIBES=-lcrypto
mytest: aes_encrypt.o ccm_mode.o  mytest.o 
	gcc   mytest.o aes_encrypt.o ccm_mode.o   -o mytest

compare:decrypt.o  aes_encrypt.o ccm_mode.o  utils.o compare.o
