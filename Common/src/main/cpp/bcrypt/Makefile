CC:=aarch64-linux-android-gcc
CXX:=aarch64-linux-android-g++ 
CFLAGS:=-O3  -std=c99 -Wall -Wextra -D_ISOC99_SOURCE -MMD -I./
CXXFLAGS:=-O3  -std=c++20 -Wall -Wextra -D_ISOC99_SOURCE -MMD -I./
mytest: aes_encrypt.o ccm_mode.o  mytest.o 
	gcc   mytest.o aes_encrypt.o ccm_mode.o   -o mytest


