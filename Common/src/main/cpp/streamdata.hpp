#pragma once
#include "SensorGlucoseData.hpp"
#include "sensoren.hpp"
extern Sensoren *sensors;

struct streamdata {
    int libreversion;
    int sensorindex;
    SensorGlucoseData *hist;
    streamdata(int libreversion,int sensorindex,SensorGlucoseData *sens):libreversion(libreversion),sensorindex(sensorindex),hist(sens) {}
    streamdata(int libreversion,int sensorindex):streamdata(libreversion,sensorindex,sensors->getSensorData(sensorindex)) {}
    streamdata(int libreversion,const char *sensorname):streamdata(libreversion,sensors->sensorindex(sensorname)) {}
    virtual bool good() const {
        return true;
        };
    virtual ~streamdata() {};

    };
struct libre3stream:streamdata {
    libre3stream(int sensindex,SensorGlucoseData *sens): streamdata(3, sensindex,sens){};
    };
#ifdef DEXCOM
struct accustream:streamdata {
    accustream(int sensindex,SensorGlucoseData *sens): streamdata(0x20, sensindex,sens){};
    };

   #include "air/air.hpp" 
struct airstream:streamdata {
        Mmap<DeviceInfo3Obj> sensorInfo;
        Mmap<air1_opcal4_arguments_t> generated;
        int tmpiter=0;
        int tmptot=0;
        int ininfo=0;
        int errors=0;
#ifndef AIR_STACK
       air_input input;
       air1_opcal4_output_t output;
       air1_opcal4_debug_t debug;
#endif
        airstream(int sensindex,SensorGlucoseData *sens): streamdata(0x30, sensindex,sens),
                                sensorInfo(hist->sensordir,sensorInfoStr,1,[](DeviceInfo3Obj *gegs){ *gegs={}; }),
                                generated(hist->sensordir,generatedStr,1)
           {
           }
        bool setNumberNew(int nr);
    };
#endif
#ifdef SIBIONICS
#include "sibionics/SiContext.hpp"
struct sistream:streamdata {
    SiContext sicontext;
    sistream(int sensindex,SensorGlucoseData *sens): streamdata(0x10, sensindex,sens),sicontext(sens){ };
    };
#endif

#ifdef DEXCOM
#include "dexcom/DexContext.hpp"
struct dexcomstream:streamdata {
    DexContext dexcontext;
    dexcomstream(int sensindex,SensorGlucoseData *sens): streamdata(0x40, sensindex,sens),dexcontext(sens){};
    };

extern bool initAidexXcrypto();

struct aidexXstream:streamdata {
        uint8_t unpair=0;
        bool unbonded=true;
        uint8_t clear=0;
        uint8_t time0=0;
        aidexXstream(int sensindex,SensorGlucoseData *sens): streamdata(0x50, sensindex,sens){
#if 0
                uint8_t key[]{0xEF,0x6F,0x46,0xF3,0xE7,0x3B,0x56,0xC7,0x80,0x21,0x34,0x21,0x6F,0xD5,0xEB,0x73};
                static_assert(sizeof(key)==16);
                memcpy(sens->getinfo()->aidexXdat.keys[0].key,key,16);
                sens->getinfo()->lastLifeCountReceived=4544;
#endif
                initAidexXcrypto();
                };
    };



#endif 
