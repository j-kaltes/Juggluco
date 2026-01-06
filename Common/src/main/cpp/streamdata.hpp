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
#endif 
