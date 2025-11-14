#pragma once
#include <string.h>
#include <algorithm>
#define LOGGERICE(...) LOGGER("ICE: " __VA_ARGS__)
#define LOGARICE(...) LOGAR("ICE: " __VA_ARGS__)
template <typename T>
class PlaceBuf {
    T *buf;
    int  maxbuf;
    int  buflen;
void increase(int last) {
     if(last>maxbuf) {
            extend(last*2);
            }
      }
void extend(int newmaxbuf) {
    auto *tmp=new T[newmaxbuf];
    memcpy(tmp,buf,buflen*sizeof(T));
    delete[] buf;
    buf=tmp;
    maxbuf=newmaxbuf;
    }
public:

    PlaceBuf(int len=1024): buf(new T[len]),maxbuf(len),buflen(0) {
        LOGGERICE("PlaceBuf::PlaceBuf(%d) buf=%p\n",len,buf);
        };

    void place(const int offset, const T *input, const int inputlen)  {
        int last=offset+inputlen;
        increase(last);
       // memcpy(buf+offset,input,inputlen*sizeof(T));
        std::copy_n(input,inputlen,buf+offset);
        buflen=std::max(last,buflen);
        LOGGERICE("place(%d,%p,%d)  buflen=%d maxbuf=%d buf=%p\n",offset,input,inputlen,buflen,maxbuf,buf);
        }
int size() const {
    return buflen;
    }
void clear() {
    buflen=0;
    }

template <typename Self>
auto *data( this Self&&self) {
    return self.buf;
    }
  void reserve(int last) {
        if(last>maxbuf) {
            extend(last);
            }
       }
  ~PlaceBuf() {
      LOGGERICE("~PlaceBuf() %p buflen=%d maxlen=%d\n",buf,buflen,maxbuf);
      maxbuf=0;
      delete[] buf;
      buf=nullptr;
      }

template <typename Self>
auto &operator[](this Self &&self,int i) {
        return self.buf[i];
        }
    };
