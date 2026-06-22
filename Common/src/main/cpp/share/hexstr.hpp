#pragma once
#include <stdio.h>
#include "inout.hpp"

class hexstr {
    static inline char null[]="null";
    char *strbuf;
    int len;
public:
       template<typename C> hexstr(const C*cont) requires requires(C t) {t.size();}: hexstr(cont?cont->data():nullptr,cont?cont->size():0) { };
       template<typename C> hexstr(const C&cont) requires requires(C t) {t.size();}: hexstr(cont.data(),cont.size()) { };
    hexstr(const uint8_t *ar ,const int arlen): strbuf(ar?(new char[arlen*2+1]):null) {
                    if(ar) {
                            int uitlen=0;
                            for(int i=0;i<arlen;i++) {
                                    uitlen+=sprintf(strbuf+uitlen,"%02X",ar[i]);
                                    }
                            strbuf[uitlen]='\0';
                            len=uitlen;
                            }
            }

        template <typename T> hexstr(const T *data):hexstr(reinterpret_cast<const uint8_t *>(data),sizeof(T)) { }
        template <typename T> hexstr(const T &data):hexstr(&data) { }
        hexstr(const data_t *data):hexstr((const uint8_t*)data->data(),data->size()) { }
        hexstr(const signed char *ar ,const int arlen): hexstr((const uint8_t*)ar,arlen) { }
        hexstr(const char *ar ,const int arlen): hexstr((const uint8_t*)ar,arlen) { }
        ~hexstr() {
                    if(strbuf!=null)
                            delete[] strbuf;
            }
    const char *str() const {	
        return strbuf;
        }
    int size() const {
        return len;
        }
        };
