#pragma once
#include <stdint.h>
#include <limits.h>
#include <stddef.h>
#include <string.h>
#include "secs.h"
extern char localestrbuf[];
struct Getopts {
    uint32_t start=0,end=0;
    int unit;
    int datnr=INT_MAX;
    bool headermode=false;
    bool jsonmode=false;

    bool streammode=false;
    bool calibratedmode=false;
    bool calibratedhistorymode=false;
    bool calibratedscansmode=false;
    bool allvaluesmode=false;
    bool pastvaluesmode=false;
    bool scansmode=false;
    bool mealsmode=false;
    bool historymode=false;
    bool amountsmode=false; 
    bool darkmode=false;
    bool exclusivemode=false;
    uint16_t lang=0;
    int width=0,height=0;
    float glow=-1.0f,ghigh=-1.0f;
int days() const {
        return (end-start+daysecs-1)/daysecs;
        }
    Getopts():unit(0) {};
    Getopts(const char *posptr,int size,int defaultduration=5*60*60);
    bool equals(Getopts &other) {
        constexpr const int len=sizeof(amountsmode)+offsetof(Getopts,amountsmode)-offsetof(Getopts,start);
        return !memcmp(&start,&other.start,len);
        }
template <int diff=60*10>
    bool aboutequal(Getopts &other,bool absolute) const {
       if(unit!=other.unit) {
/*
          if(!unit) unit=settings->data()->unit; 
          if(!other.unit) other.unit=settings->data()->unit; 
          if(unit!=other.unit)*/ {
              LOGGER("aboutequal: unit different %d!=%d\n",unit,other.unit);
              return false;
              }
          }
        uint16_t lan1=getlan(lang);
        uint16_t lan2=getlan(other.lang);
        if(lan1!=lan2) {
              LOGGER("aboutequal: language different %.2s!=%.2s\n",(const char *)&lan1,(const char*)&lan2);
                return  false;
                }
        if(darkmode!=other.darkmode) {
                LOGGER("aboutequal: darkmode diffferent %d!=%d\n",darkmode,other.darkmode);
                return false;
                }
        if(calibratedmode!=other.calibratedmode) {
                LOGGER("aboutequal: calibratedmode diffferent %d!=%d\n",calibratedmode,other.calibratedmode);
                return false;
                }
        if(calibratedhistorymode!=other.calibratedhistorymode) {
                LOGGER("aboutequal: calibratedhistorymode diffferent %d!=%d\n",calibratedhistorymode,other.calibratedhistorymode);
                return false;
                }
        if(calibratedscansmode!=other.calibratedscansmode) {
                LOGGER("aboutequal: calibratedscansmode diffferent %d!=%d\n",calibratedscansmode,other.calibratedscansmode);
                return false;
                }
        if(allvaluesmode!=other.allvaluesmode) {
                LOGGER("aboutequal: allvaluesmode diffferent %d!=%d\n",allvaluesmode,other.allvaluesmode);
                return false;
                }
        if(pastvaluesmode!=other.pastvaluesmode) {
                LOGGER("aboutequal: pastvaluesmode diffferent %d!=%d\n",pastvaluesmode,other.pastvaluesmode);
                return false;
                }
       if(!absolute) {
            if(differs(end,other.end,diff))
                  return false;
             }
        else {
             if(other.end!=end)
                  return false;
             }
        if(width!=other.width) {
                LOGGER("width different %d!=%d\n",width,other.width);
                return false;
                }
        if(height!=other.height) {
                LOGGER("height different %d!=%d\n",height,other.height);
                return false;
                }
        if(glow!=other.glow) {
                LOGGER("glow different %d!=%d\n",glow,other.glow);
                return false;
                }
        if(ghigh!=other.ghigh) {
                LOGGER("ghigh different %d!=%d\n",ghigh,other.ghigh);
                return false;
                }
        return !(differs(start,other.start,diff)); 
        }
private:
static uint16_t getlan(uint16_t l) {
        return l==0?*((uint16_t*)localestrbuf):l;
        }
static   bool differs(uint64_t one,uint64_t two,int val) {
        const int64_t diff=one-two;
        const auto res=diff>val||diff<-val;
        LOGGER("aboutequal: differs(%u,%u,%d)=%d\n",one,two,val,res);
        return res;
        }
    };
