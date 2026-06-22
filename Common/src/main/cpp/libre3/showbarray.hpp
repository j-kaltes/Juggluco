#pragma once
#include <stdio.h>
#include "jniclass.hpp"



struct showbarray {
inline static char null[]="null";
int len;
char *str;
showbarray(JNIEnv *env,jbyteArray jar):len(jar?env->GetArrayLength(jar):0),str(jar?new char[len*3+1]:null) {
    if(!jar) {
        return;
        }
        
    CritAr<uint8_t> input(env,jar); 
    char *iter=str;
    auto *data=input.data();
    for(int i=0;i<len;++i) {
        iter+=sprintf(iter,"%02hhX ",data[i]);
        }
    }
~showbarray() {
    if(str!=null)
        delete[] str;
    }
    };
