#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <stdint.h>
#include <string_view>
#include <unistd.h>

#include <inttypes.h>
#define SIBIONICS 
#include "inout.hpp"
#include "share/hexstr.hpp"
#include "logs.hpp"
#include "jnisubin.hpp"
#include "air.hpp"
#include "SensorGlucoseData.hpp"

#include "streamdata.hpp"
union {
    air1_opcal4_device_info_t device;
    #include "run6_0.h"
    } run6_0;

union  {
        struct  {
            uint8_t start[752];
            union {
                struct air1_opcal4_cgm_input_t cgm_input[];
                #include "run6_1.h"
                } __attribute__ ((packed));
            } __attribute__ ((packed));
     air1_opcal4_cal_list_t cal_list[];
     }  __attribute__ ((packed))  around;
union  {
    #include "run6_3.h"
     air1_opcal4_arguments_t generated;
    } run6_3;

using namespace std::literals;
std::string_view libdirname{"./"};
extern std::string_view libdirname;

extern "C" jint         subRegisterNatives(JNIEnv*, jclass name, const JNINativeMethod*methods, jint nr);
jint         subRegisterNatives(JNIEnv*, jclass name, const JNINativeMethod*methods, jint nr) {
    LOGAR("subRegisterNatives");
        return 0;
        }

int getgetsendnr() {
        return 0;
        }
#ifdef DYNAMIC
void *openlib(std::string_view libname) {
    int liblen=libdirname.size();
    if(liblen<=0) {
      LOGGER("libdirname.size()=%d\n",liblen);
      return  nullptr;
      }
    int libnamelen=libname.size()+1;
    char fullpath[libnamelen+ liblen];
    memcpy(fullpath,libdirname.data(),liblen);
    memcpy(fullpath+liblen,libname.data(),libnamelen);
    LOGGER("open %s\n",fullpath);
    return dlopen(fullpath, RTLD_NOW);
    }
#define algtype(x) x##_t


typedef unsigned char (*air1_opcal4_algorithm_t)(air1_opcal4_device_info_t *, air1_opcal4_cgm_input_t *, air1_opcal4_cal_list_t *, air1_opcal4_arguments_t *, air1_opcal4_output_t *, air1_opcal4_debug_t *);
air1_opcal4_algorithm_t air1_opcal4_algorithm;
static bool getlibfuncs() {
    std::string_view libcal{"libCALCULATION.so"};
    void *handle=openlib(libcal);
    if(!handle) {
        LOGGER("dlopen %s failed: %s\n",libcal.data(),dlerror());
        return false;
        }
   constexpr const char str[]="air1_opcal4_algorithm";
   air1_opcal4_algorithm= (air1_opcal4_algorithm_t) dlsym(handle,str);
   if(!air1_opcal4_algorithm) {
        LOGGER("dlsym %s failed: %s\n",str,dlerror());
        return false;
        }
    return true;
    };

#else
extern "C" unsigned char air1_opcal4_algorithm( struct air1_opcal4_device_info_t *dev_info, struct air1_opcal4_cgm_input_t *cgm_input, struct air1_opcal4_cal_list_t *cal_input, struct air1_opcal4_arguments_t *algo_args, struct air1_opcal4_output_t *algo_output, struct air1_opcal4_debug_t *algo_debug);

#endif
extern struct JNINativeInterface envbuf;
using objectarray=gegs<jobject,4>;
/*
struct objectarray {
    int len;
    jobject obj[4];
    }; */
//4320*(155 + 74) + 1 + 446 + 1579 + 117312

int getsize(objectarray   &ar)  {
    int totlen=0;
    for(int i=0;i<ar.len;++i) {
        totlen+= alignstart<int64_t>(4+ ((data_t *)ar.data()[i])->size());
        }
     int  elements=((data_t *)ar.data()[1])->size()/74;
     totlen+=alignstart<int64_t>( elements*155+4)+alignstart<int64_t>(1579+4)+alignstart<int64_t>(1+4)+8;
     return totlen;
    }
//   objectarray   inobj{4,(jobject)&run6_0,(jobject)&run6_1 ,(jobject)&run6_2 ,(jobject)&run6_3};
extern void   showRUNar(int index,const uint8_t *bytes,int len);

//extern unsigned char air1_opcal4_algorithm(air1_opcal4_device_info_t *, air1_opcal4_cgm_input_t *, air1_opcal4_cal_list_t *, air1_opcal4_arguments_t *, air1_opcal4_output_t *, air1_opcal4_debug_t *);
std::string_view globalbasedir="basedir";

         void   SensorGlucoseData::prunedata() {}
struct inputdata_t {
    uint32_t time;
    std:: vector<uint8_t> data;
    };

inputdata_t inputdata[]{
#include "data.hpp"
};
template <typename T>
     char *   printarray(char *ptr,char *end,T *numbers,const int len) {
        for(int i=0;i<(len-1);++i) {
                auto res=std::to_chars(ptr,end,numbers[i]); 
                ptr=res.ptr;
                *ptr++=',';
                *ptr++=' ';
                };
        auto res=std::to_chars(ptr,end,numbers[len-1]); 
        *res.ptr='\0';
        return res.ptr;
        }
static void showm(const  m *mptr) {
    time_t time=mptr->measurement_time;
    constexpr int maxbuf=400;
    char buf[maxbuf];
    char *endbuf=buf+maxbuf;
    int endpos=snprintf(buf,maxbuf,"m seq_num %d temperature %.1f unixtime=%lu: ", mptr->sequence_number,mptr->temperature,time);
    char *end=printarray(buf+endpos,endbuf,mptr->glucose_array.data(),30);
    *end++=' ';
    ctime_r(&time,end);
    LOGGERN(buf,end-buf+24);
    }

static jlong glucoselong(uint32_t nu,uint32_t glval,float drate,const SensorGlucoseData *hist) {
        if(!glval) 
            return 0LL;
        const jlong rate=roundl(((long double)drate)*1000LL);
 //       auto calibrate= make_calibrator<ScanData>(hist);
  //      const double cali=calibrate.calibrateNow(nu,glval);
        uint32_t mgL=glval*10;
        /*
        if(!isnan(cali)) {
            mgL=(uint32_t)round(cali*10.0);
            }
        else 
            mgL=glval*10;
*/
//        const jlong alarmcode= getalarmonly(mgL,drate,hist);
        const jlong alarmcode= 0;
        const jlong res= (rate&0xFFFF)<<32|alarmcode|mgL;
        LOGGER("glucoselong=%" PRId64 "\n",res);
        return res;
        }
extern jlong glucoseback(uint32_t nu,uint32_t glval,float drate,SensorGlucoseData *hist);
jlong glucoseback(uint32_t nu,uint32_t glval,float drate,SensorGlucoseData *hist) {
        if(!glval) return 0LL;
        hist->setbluetoothOn(1);
        auto res= glucoselong(nu,glval,drate,hist);
        hist->waiting=false;
        return res;
        }

jlong mkres(SensorGlucoseData *sens,uint32_t timsec,uint32_t eventTime, int min,int mgdL, int abbotttrend, float change){
    if(sens->savestreamonly(eventTime,min,mgdL,abbotttrend, change)) {
            jlong res;
            if((timsec-eventTime)<maxbluetoothage) {
                 sens->sensorerror=false;
                 const int sensorindex=sens->sensorIndex;
/*                 sensor *sensor=sensors->getsensor(sensorindex);
                 if(sensor->finished) {
                        LOGGER("mkres %s finished was %d becomes 0\n", sens->showsensorname().data(),sensor->finished);
                        sensor->finished=0;
                       // backup->resensordata(sensorindex);
                        } */
                 res=glucoseback(eventTime,mgdL,change,sens);
                 //wakewithcurrent();
                 }
              else {
                sens->receivehistory=timsec;
                res=1LL;
                }
            // backup->wakebackup(wakestream);
             return res;
             }
     return 0LL;
    }

extern int rate2changeindex(float rate);
int rate2changeindex(float rate) {
        if (rate <= -2.0f) {
            return 1;
        }
        if (rate <= -1.0f) {
            return 2;
        }
        if (rate <= 1.0f) {
            return 3;
        }
        if (rate <= 2.0f) {
            return 4;
        }
        if(std::isnan(rate)) {
            return 0;
        }
        return 5;
    }
#include "glucose.hpp"

void       airsavehistory( SensorGlucoseData *sens, air1_opcal4_output_t &output) {
     int lastid=output.seq_number_final;
     uint32_t lasttime=output.measurement_time_standard;

     int firstid=0,endhistory=0;
     for(int i=0;i<6;++i) {
        int id=output.smooth_seq[i];
        double mgdL=output.smooth_result_glucose[i];
        if(id&&mgdL>0.0) {
             if(!firstid)
                firstid=id;
             uint32_t time=(id-lastid)*5*60+lasttime;
             Glucose *item=sens->getglucose(id);
             item->time=time;
             auto lifeCount=id*5;
             item->id=lifeCount;
             auto mgL=std::round(mgdL*10.0);
             item->glu[1]=mgL;
             #ifndef NOLOG
             time_t wastime=time;
             LOGGER("airsavehistory(%d,%d,%.1f) %s",id,lifeCount,mgL/convfactor,ctime(&wastime));
             #endif
             endhistory=id;
             }
        }
     {
     double mgdLdouble=output.result_glucose;
     if(mgdLdouble>0.0) {
         int mgL= std::round(mgdLdouble*10.0);
         Glucose *item=sens->getglucose(lastid);
         item->time=lasttime;
         auto lifeCount=lastid*5;
         item->id=lifeCount;
         item->glu[1]=mgL;
         #ifndef NOLOG
         time_t wastime=lasttime;
         LOGGER("airsavehistory(%d,%d,%.1f) %s",lastid,lifeCount,mgL/convfactor,ctime(&wastime));
         #endif
         endhistory=lastid;
         if(!firstid)
                firstid=lastid;
         }
     }
    if(firstid) {
        if(sens->getstarthistory()<=0) {
            int i=firstid-1;
            for(;i>0&&sens->getglucose(i)->id;--i) 
                ;
            sens->setstarthistory(i+1);
            }
        ++endhistory;
        if(endhistory>sens->getScanendhistory())
            sens->setendhistory(endhistory);
//        sens->backhistory(firstid);
        };

    }

bool airstream::setNumberNew(int nr) {
//    if(tmptot) return false;
   //  data_t::deleteex (tmpinput);
//     const int totsize=nr*74;
     LOGGER("airstream::setNumberNew(%s,%d)\n",hist->showsensorname().data(),nr);
     tmpiter=0;
     tmptot=nr;
     ininfo=0;
   //  tmpinput=data_t::newex(totsize);
     return true;
    }
jlong airProcessData(airstream *sdata,const jbyte *indata,int arlen,jlong *timeres) {
     SensorGlucoseData *sens=sdata->hist;
      if(!sens) {
          LOGAR("airProcessData SensorGlucoseData==null");
          return 1LL;
         }
    const AirData *air=reinterpret_cast<const AirData *>(indata);
    if(air->reg1!=1) {
        LOGGER("airProcessData second byte %d not 1\n",air->reg1);
        return 1LL;
        }
    jlong msec=timeres[0];
    uint32_t nowsec= msec/1000LL;
    if(air->reg0==0xC4) {
        const int newrecords=air->numRecords;
        sdata->setNumberNew(newrecords);
        if(!newrecords&&!sens->getinfo()->askEarlier) {
            if(const ScanData *last=sens->lastpoll()) {
                if((nowsec-last->gettime())>60*6) {
                     LOGAR("set askEarlier=1");
                     sens->getinfo()->askEarlier=1;
                     return 2LL;
                     }
                }
            }
         return 3LL;
        }
    if(air->reg0!=0xC5) {
        LOGGER("airProcessData first byte %d not 197\n",air->reg0);
        return 1LL;
        }
     if(arlen<sizeof(AirData))  {
        LOGGER("airProcessData size  value %d < AirData %zd\n",arlen,sizeof(AirData));
//        sens->sensorerror=true;
 //       sens->sensorErrorTime=timsec;
        return 1LL;
        }
     ++sdata->tmpiter;
    if(!air->deviceErrorCode) {
#ifdef DYNAMIC
        static bool haslibs=getlibfuncs();
#endif
        auto mtime=air->time;
        if(mtime<31532400) {
            if(sdata->tmptot>1)
                mtime=nowsec - (sdata->tmptot - sdata->tmpiter ) * 300;
             else
                mtime=nowsec;
            }

       const int idnow=air->sequenceNumber;
#ifdef AIR_STACK
       air_input input{.data={.sequence_number=static_cast<uint16_t>(idnow),.measurement_time=mtime,.glucose_array=air->glucose_array,.temperature=air->temperature/100.0}};
       air1_opcal4_output_t output{};
       air1_opcal4_debug_t debug{};
#else
     air_input &input=sdata->input;
        input={.data={.sequence_number=static_cast<uint16_t>(idnow),.measurement_time=mtime,.glucose_array=air->glucose_array,.temperature=air->temperature/100.0}};
       air1_opcal4_output_t &output=sdata->output; sdata->output={};
       air1_opcal4_debug_t &debug=sdata->debug; sdata->debug={};
#endif
       showm(&input.data);



       DeviceInfo3Obj *deviceInfo=sdata->sensorInfo.data();

       unsigned char res=air1_opcal4_algorithm(reinterpret_cast<air1_opcal4_device_info_t *>(deviceInfo), &input.cgm_input, &input.empty, sdata->generated.data(), &output, &debug);
       if(res&&!output.errcode) {
        
            double mgdLdouble=output.result_glucose;
            double mmolL=mgdLdouble/18.0;
            int mgdL= std::round(mgdLdouble);
            double trendrate=output.trendrate;
            if(trendrate>99.0)
                trendrate=NAN;
            int abbotttrend=rate2changeindex(trendrate);
            int id=output.seq_number_final*5;
            uint32_t time=output.measurement_time_standard;
            time_t tim=time;
            #ifndef NOLOG
           const char *label=abbotttrend<6?GlucoseNow::trendString[abbotttrend]:"Error";
            LOGGER("airProcessData: nr=%d id=%d glucose=%.1f mg/dL %.1f mmol/L trendrate=%.2f %s (%d) %u  %s", output.seq_number_final, id,mgdLdouble,mmolL,trendrate,label,abbotttrend,time,ctime(&tim));
            #endif
            auto res=mkres(sens,nowsec,time,id, mgdL,  abbotttrend, trendrate);
            if(!res) {
                if(mgdL) {
                    sens->getinfo()->askEarlier=0;
                    LOGAR("set askEarlier=0");
                    res=2LL;
                    }
                }
            airsavehistory(sens,output);
            timeres[0]=time*1000LL;
            return res;
            }
        else {
                LOGGER("airProcessData: air1_opcal4_algorithm res=%d output.errcode=%d\n",(int)res,output.errcode);
                }
       }
     else {
            LOGGER("airProcessData: air->deviceErrorCode=%d\n",air->deviceErrorCode);
        }
    return 0LL;
    }
int main() {
#ifdef DYNAMIC
    getlibfuncs();
#endif
mkdir("basedir",0700);
//inputdata_t el{1766955625,{0xC4,0x01,0xAE,0x00}};
std::string_view scanned{"01088067120060481726050621C1Q470A02339240706546250000000250411B001"};
std::string_view sensordir{"basedir/2605C1Q470A02339"};
uint32_t then=1766498240;
 SensorGlucoseData::mkdatabaseAir(sensordir,scanned,then);
 int sensorindex=1234;
 SensorGlucoseData *sens=new SensorGlucoseData(sensordir,sensorindex);
  auto stream = new airstream(sensorindex, sens);

        memcpy(stream->sensorInfo.data(),&run6_0.device,446);
    for(const auto&el:inputdata) {
        jlong ar[1]{el.time};
        auto res= airProcessData(stream,(jbyte*)el.data.data(),el.data.size(),ar);
        
        }
    };

