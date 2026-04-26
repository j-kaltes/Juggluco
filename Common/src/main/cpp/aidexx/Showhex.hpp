#pragma once
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <algorithm>
inline int showhex(uint8_t *input,int len,char *output) {
    if(!input) {
        const int lennull=std::min(4,len*2);
        memcpy(output,"null",lennull+1);
        return lennull;
        }
    char *ptr=output;
    for(int i=0;i<len;i++) {
        ptr+=sprintf(ptr,"%02X",input[i]);
        }    
    return ptr-output;
    }
struct Showhex {
    int uitlen;
    char *data;    
    Showhex(const void *input,int len):uitlen(len*2),data(new char[uitlen+1]) {
         if(!len) { 
                *data='\0';
                }
         else
                uitlen=showhex((uint8_t*)input,len,data);
        }
    operator const char *() const {
        return data;
        }
    int size() const {
        return uitlen;
        }
    ~Showhex() {
        delete[] data;
        }

    };
