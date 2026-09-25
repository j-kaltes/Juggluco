#!/usr/bin/env python3
"""Run the production first NFC response parser with synthetic JNI arrays under sanitizers."""
from pathlib import Path
import subprocess, tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'Common/src/main/cpp/libre3/nfc.cpp').read_text()
start=source.index('struct firstnfc {'); end=source.index('static_assert',start)
parser=source[start:end]
headers=root/'Common/src/main/cpp/libre3'
# The NG translation unit includes its shared header before firstnfc.
header='nfcmetadata.hpp' if (headers/'nfcmetadata.hpp').exists() else 'nfcresponse.hpp'
prefix=r'''
#include <vector>
#include <string_view>
#include <cassert>
#include <cstring>
#include <cstdint>
#include <cstdio>
#define NOLOG 1
#define LOGGER(...) ((void)0)
#define LOGGERN(...) ((void)0)
#define LOGAR(...) ((void)0)
using jbyte=signed char;using jsize=int;using jbyteArray=std::vector<jbyte>*;
struct JNIEnv {
 bool fail=false;
 int GetArrayLength(jbyteArray a){assert(a);return a->size();}
 void GetByteArrayRegion(jbyteArray a,int start,int len,jbyte* out){assert(start+len<=a->size());if(!fail)std::memcpy(out,a->data()+start,len);}
 bool ExceptionCheck(){return fail;}
};
'''
tests=r'''
int main(){
 JNIEnv env;
 std::vector<jbyte> valid(3+sizeof(firstnfc),0);valid[1]=static_cast<jbyte>(0xA5);
 // Construct over deliberately dirty storage to expose success-path error initialization.
 alignas(nfc1) unsigned char storage[sizeof(nfc1)];
 for(int pattern: {0,1,127,255}) {std::memset(storage,pattern,sizeof storage);nfc1* result=new(storage)nfc1(&env,&valid);assert(!result->error);result->~nfc1();}
 nfc1 nullResult(&env,nullptr);assert(nullResult.error);
 for(size_t n=0;n<valid.size();++n){std::vector<jbyte> shortReply(valid.begin(),valid.begin()+n);nfc1 p(&env,&shortReply);assert(p.error);}
 auto repeated=valid;repeated.insert(repeated.begin()+2,static_cast<jbyte>(0xA5));nfc1 repeat(&env,&repeated);assert(!repeat.error);
 auto bad=valid;bad[0]=1;nfc1 badStatus(&env,&bad);assert(badStatus.error);
 bad=valid;bad[1]=0;nfc1 badMagic(&env,&bad);assert(badMagic.error);
 bad=valid;bad[2]=1;nfc1 badTerminator(&env,&bad);assert(badTerminator.error);
 std::vector<jbyte> runaway(76,static_cast<jbyte>(0xA5));runaway[0]=0;nfc1 noTerminator(&env,&runaway);assert(noTerminator.error);
 std::vector<jbyte> tooLarge(4096,0);tooLarge[1]=static_cast<jbyte>(0xA5);nfc1 overflow(&env,&tooLarge);assert(overflow.error);
 env.fail=true;nfc1 failedRead(&env,&valid);assert(failedRead.error);
 puts("PASS: production NFC parser, dirty-storage success, null, truncation, prefix bounds, oversize and JNI failure");
}
'''
with tempfile.TemporaryDirectory(prefix='libre3-nfc-response-') as d:
 d=Path(d);src=d/'parser.cpp';src.write_text(prefix+'\n#include <new>\n#include "'+header+'"\n'+parser+tests)
 exe=d/'parser';subprocess.run(['g++','-std=c++20','-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie','-g','-I',str(headers),str(src),'-o',str(exe)],check=True);subprocess.run([str(exe)],check=True)
