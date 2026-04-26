
#ifdef SIBIONICS
#include "config.h"
#ifdef NOTCHINESE
#include <string_view>
#include <jni.h>
#include <memory>
#include <inttypes.h>
#include "SensorGlucoseData.hpp"
#include "fromjava.h"
#include "logs.hpp"
//#include "sibionics/AlgorithmContext.hpp"
#include "streamdata.hpp"
#include "datbackup.hpp"
#include "inout.hpp"
#include "jnidef.h"
#include "EverSense.hpp"
/*
static auto reverseaddress(const char address[]) {
   std::array<jbyte,6> uitar;
   auto *uit=uitar.data();
   sscanf(address,"%hhX:%hhX:%hhX:%hhX:%hhX:%hhX",uit+5,uit+4,uit+3,uit+2,uit+1,uit);
    return uitar;
   } */

/*
std::array<jbyte,6>  deviceArray(const char address[]) {
   std::array<jbyte,6> uitar;
   auto *uit=uitar.data();
   sscanf(address,"%hhX:%hhX:%hhX:%hhX:%hhX:%hhX",uit+5,uit+4,uit+3,uit+2,uit+1,uit);
   return uitar;
   } */

#ifndef NOLOG
extern void logbytes(std::string_view text,const uint8_t *value,int vallen) ;
#else
#define logbytes(text,value, vallen) 
#endif

extern JNIEnv *subenv;

#include "share/hexstr.hpp"

std::pair<std::unique_ptr<data_t>,int> getActivation(jlong timesec) {
    auto zero=data_t::newex(2);
    zero->clear();
    auto fill=data_t::newex(50);
   int ret=V120Activation(subenv, nullptr,0, true, (jbyteArray) zero, timesec, 1234, (jbyteArray) fill, fill->size());
#ifndef NOLOG
   hexstr zerohex((uint8_t*)zero->data(),zero->size());
   hexstr fillhex((uint8_t*)fill->data(),ret);
   LOGGER("getActivation(%jd) zero=%s res=%s\n",timesec,zerohex.str(),fillhex.str());
#endif
   data_t::deleteex(zero);
   return {std::unique_ptr<data_t>(fill),ret};
   }
extern "C" JNIEXPORT jbyteArray JNICALL   fromjava(getSIActivation)(JNIEnv *env, jclass cl) {
   auto [cmd,len]=getActivation(time(nullptr));
   jbyteArray uit=env->NewByteArray(len);
   env->SetByteArrayRegion(uit, 0, len,cmd.get()->data());
   return uit;
   }


extern "C" JNIEXPORT jbyteArray JNICALL   fromjava(getSIResetBytes)(JNIEnv *env, jclass cl) {
    auto zero=data_t::newex(2);
    zero->clear();
    auto fill=data_t::newex(1024);
    int uitlen=V120Reset(subenv, nullptr,0, true, (jbyteArray) zero, 0, (jbyteArray) fill, fill->size());
    #ifndef NOLOG
    hexstr fillhex((uint8_t*)fill->data(),uitlen);
    LOGGER("V120Reset %s\n",fillhex.str());
    #endif
    jbyteArray uit=env->NewByteArray(uitlen);
    env->SetByteArrayRegion(uit, 0, uitlen,fill->data());
    data_t::deleteex(zero);
    data_t::deleteex(fill);
    return uit;
    }




static Data_t getIsecUpdate(jlong timesec) {
    Data_t zero(2);
    zero.clear();
    Data_t fill(50);
   int ret=V120IsecUpdate(subenv, nullptr,0, true,  zero, timesec, fill, fill.data->size());
#ifndef NOLOG
   hexstr zerohex((uint8_t*)zero.data->data(),zero.data->size());
   hexstr fillhex((uint8_t*)fill.data->data(),ret);
   LOGGER("getIsecUpdate(%jd) zero=%s res=%s\n",timesec,zerohex.str(),fillhex.str());
#endif
   fill.used=ret;
   return fill;
   }
extern "C" JNIEXPORT jbyteArray JNICALL   fromjava(getSItimecmd)(JNIEnv *env, jclass cl) {
   auto cmd=getIsecUpdate(time(nullptr));
   int len=cmd.used;
   jbyte *dat=cmd.data->data();
   jbyteArray uit=env->NewByteArray(len);
   env->SetByteArrayRegion(uit, 0, len,dat);
   return uit;
   }

 static void  keysRegistered(int hema) {
    static constexpr const  struct {
          std::string_view appid;
          std::string_view key;
          } appgegs[] {
   {"com.sisensing.sijoy"sv,      "56CE249349040C94F8B4B2375A8752D5CBE7A17814B502D9132489C0BFDFC99F0CAC670E8CBB085AF1C780B3D282E3"sv},  //EU
   {"com.sisensing.rusibionics"sv, "60B05FEB7C0A148DEED2B3375A8754D9D0E6A5751BCE02D9132489C0BFDFC99F0CAC670E8DA7115CEACF87B7DE8FD4612E1B7638C2"sv}, // Hematonix
   {"com.sisensing.sisensingcgm"sv,"4E8E1CAF43051F97EEC9C1475A8752D5C387D17A65B002D9132489C0BFDFC99F0CAC670E8CBB1150E6D581B7D08FC03404052C57AD58"sv}, //Chinese
   {"com.sisensing.eco"sv,         "068449FA5C1B1F97EEC9C1475A8752D5C387D17A65B002D9132489C0BFDFC99F0CAC670E9AB10D62FDE0B2B1E7"sv}, //Sibionics 2
   {"com.sisensing.gs3"sv,         "46C04E9267430C94F8B4B2375A8752D5CBE7A17814B502D9132489C0BFDFC99F0CAC670E98A1510AEA9186FAC6"sv}
                }; 
   const auto &gegs=appgegs[hema]; 

   data_t *sijkey=data_t::newex(gegs.key);
   data_t *name=data_t::newex(gegs.appid);
#ifndef NOLOG
   LOGGER("v120RegisterKey %.*s %.*s size=%d \n",sijkey->size(),sijkey->data(),name->size(),name->data(),name->size());
#endif
   v120RegisterKey(subenv,nullptr,(jbyteArray)sijkey, sijkey->size(), (jbyteArray)name);
   LOGAR("na v120RegisterKey");
   data_t::deleteex(name);
   data_t::deleteex(sijkey);
   }


std::array<jbyte,6>  deviceArray(const char address[]);

auto makeauthbytes(const char * address,int hema) {
   keysRegistered(hema);
   auto rev=deviceArray(address);
   Data_t jrev(rev);
   Data_t uitar(50); 
   int uitlen = V120ApplyAuthentication(subenv,nullptr,1, true, 0,jrev , uitar, uitar.capacity());
   uitar.used=uitlen;
   return uitar;
   }
extern "C" JNIEXPORT jint JNICALL   fromjava(getSensorptrSiSubtype)(JNIEnv *env, jclass cl,jlong sensorptr) {
   if(!sensorptr) {
       LOGAR("getSensorptrSiSubtype sensorptr==0");
       return -1;
       }
   return reinterpret_cast<const SensorGlucoseData*>(sensorptr)->siSubtype();
   }
extern "C" JNIEXPORT void JNICALL   fromjava(setSensorptrSiSubtype)(JNIEnv *env, jclass cl,jlong sensorptr,jint type) {
   if(!sensorptr) {
       LOGAR("setSiSubtype sensorptr==0");
       return;
       }
   LOGGER("setSensorptrSiSubtype %d\n",type);
   auto *usedhist=reinterpret_cast<SensorGlucoseData*>(sensorptr);
   usedhist->getinfo()->siType=type;
   if(type!=3) {
        usedhist->getinfo()->reset=false;
        }
   LOGGER("after usedhist->getinfo()->siType %d\n",type);
   sendsiScan(usedhist);
   LOGAR("sendsiScan(usedhist)");
   if(backup)
       backup->wakebackup(wakeall);
   LOGAR("wakebackup(wakeall)");

   }
extern "C" JNIEXPORT jint JNICALL   fromjava(getSiSubtype)(JNIEnv *env, jclass cl,jlong dataptr) {
   if(!dataptr) {
       LOGAR("getSiSubtype dataptr==0");
       return -1;
       }
   const SensorGlucoseData *usedhist=reinterpret_cast<streamdata *>(dataptr)->hist ; 
   if(!usedhist) {
       LOGAR("getSiSubtype usedhist==null");
       return -1;
       }

   if(usedhist==Sensoren::isdeleted)  {
        LOGAR("getSiSubtype isdeleted");
        return -1;
        }
   return  usedhist->siSubtype();
   }
   /*
extern "C" JNIEXPORT void JNICALL   fromjava(setSiSubtype)(JNIEnv *env, jclass cl,jlong dataptr,jint type) {
   if(!dataptr) {
       LOGAR("setSiSubtype dataptr==0");
       return;
       }
   SensorGlucoseData *usedhist=reinterpret_cast<streamdata *>(dataptr)->hist ; 
   if(!usedhist) {
       LOGAR("setSiSubtype usedhist==null");
       return;
       }
   LOGGER("setSiSubtype %d\n",type);
   usedhist->getinfo()->siType=type;
   LOGGER("after usedhist->getinfo()->siType %d\n",type);
   sendsiScan(usedhist);
   LOGAR("sendsiScan(usedhist)");
   if(backup)
       backup->wakebackup(wakeall);
   LOGAR("wakebackup(wakeall)");

   }
*/
extern "C" JNIEXPORT jbyteArray JNICALL   fromjava(siAuthBytes)(JNIEnv *env, jclass cl,jlong dataptr) {
   if(!dataptr) {
       LOGAR("siAuthBytes dataptr==null");
       return nullptr;
       }
   const SensorGlucoseData *usedhist=reinterpret_cast<streamdata *>(dataptr)->hist ; 
   if(!usedhist) {
       LOGAR("siAuthBytes usedhist==null");
       return nullptr;
       }
   const auto address=usedhist->deviceaddress();
   const auto data=makeauthbytes(address,usedhist->siSubtype());
   const auto *dat=data.data->data();
   const int len=data.used;
   logbytes("siAuthBytes",(const uint8_t *)dat,len);
   jbyteArray uit=env->NewByteArray(len);
   env->SetByteArrayRegion(uit, 0, len,dat);
   return uit;
   }

extern Data_t askindexdata(jlong index);
Data_t askindexdata(jlong index) {
   Data_t dat(30);
   Data_t zero(2);
  int res=V120RawData(subenv, nullptr, 0, true, (jbyteArray)zero, index, 0, (jbyteArray)dat, dat.capacity());
   dat.used=res;
  return dat;
   }



#ifdef TEST
#define THREADLOCAL 
#else
#define THREADLOCAL  thread_local
#endif
static THREADLOCAL jlong sprintargs[2048];
static  THREADLOCAL int recordsprint=-1;
#define VISIBLE __attribute__((__visibility__("default")))
extern "C" int VISIBLE  __vSprintf_chk(char * s, int flag, size_t slen, const char * format, va_list args);
extern "C" int VISIBLE  __vsprintf_chk(char * s, int flag, size_t slen, const char * format, va_list args);
extern "C" int VISIBLE  __vSprintf_chk(char * s, int flag, size_t slen, const char * format, va_list args) {
   if(recordsprint>=0) {
       va_list newargs;
       va_copy(newargs, args);
      jlong val=va_arg(newargs, jlong);
      sprintargs[recordsprint++]=val;
      va_end(newargs);
      } 
   int res=__vsprintf_chk( s, flag, slen, format,  args);
   LOGGER(" __vsprintf_chk(%s (%p),%d,%zd,%s,va_list)=%d\n",s,s,flag,slen,format,res); 
   return res;
   }
#include <vector>
extern int sitrend2abbott(int sitrend) ;
extern float sitrend2RateOfChange(int sitrend);

extern uint32_t makestarttime(int index,uint32_t eventTime);

//#include "sibionics/json.hpp"
//extern bool savejson(SensorGlucoseData *sens,std::string_view, int index,const AlgorithmContext *alg,getjson_t getjson );
//extern getjson_t getjson2;
//extern jlong glucoseback(uint32_t glval,float drate,SensorGlucoseData *hist) ;
extern jlong glucoseback(uint32_t nu,uint32_t glval,float drate,SensorGlucoseData *hist) ;

jlong SiContext::processData2(SensorGlucoseData *sens,time_t nowsecs,data_t *data,int sensorindex)  {
   const int datasize=data->size();
   logbytes("processData2 input: ",(const uint8_t *)data->data(),datasize);
   Gegs<jint> jiar(2);
   Data_t jsonuit(7168);
   Data_t bar2(2);

   memset(jiar.data->data(),'\0',sizeof(jint)*jiar.data->size());
   memset(bar2.data->data(),'\0',bar2.data->size());
   recordsprint=0;
  int nritems=V120SpiltData(subenv,nullptr,0,(jbyteArray)data,(jintArray)jiar,(jbyteArray)jsonuit,true,(jbyteArray)bar2,datasize);
  if(nritems<=0) {
     nritems=V120SpiltData(subenv,nullptr,0,(jbyteArray)data,(jintArray)jiar,(jbyteArray)jsonuit,false,(jbyteArray)bar2,datasize);
     }
//   int recorded=recordsprint;
  recordsprint=-1;
   LOGGER("nritems=%d\n",nritems);
   LOGGER("%s\n",jsonuit.data->data());
   logbytes("bar2",(const uint8_t *)bar2.data->data(),bar2.data->size());
   int *idat=jiar.data->data();
   #ifndef NOLOG
{   char tmpbuf[80];
   const char str[]="jiar:";
   memcpy(tmpbuf,str,sizeof(str)-1);
   char *ptr=tmpbuf+ sizeof(str)-1;
   for(int i=0;i<jiar.data->size();i++) {
      ptr+=sprintf(ptr," %d",idat[i]);
      }
   *ptr++='\n';
   LOGGERN(tmpbuf,ptr-tmpbuf);
   }
   #endif
  jlong *basear=sprintargs;
  switch(idat[0]) {
    case  0xC007: { // 49159: 
      sensor *sensor=sensors->getsensor(sensorindex);
      for(int i=0;i<nritems;i++) {
        int maxid=sens->getSiIndex();
        int index=(int) basear[0];
        time_t eventTime=basear[10];
        if(eventTime>nowsecs)
            eventTime=nowsecs;
        if(index!=maxid)   {
                if(index<maxid)   {
                   LOGGER("SIprocess index=%d<maxid=%d\n",index,maxid);
                   uint32_t lasttime=sens->getlastpolltime();
                   if(eventTime>lasttime) {
                      int larger=maxid-index; 
                      int add2index=5*((larger+4)/5);
                      sens->setSiAdd2Index(add2index);
                      maxid=index;
                      sens->setSiIndex(index);
                      LOGGER("index=%d setSiAdd2Index(add2index=%d)\n",index,add2index);
                      }
                   else {
                       return 3LL;
                       }
                   }
                else {
                   LOGGER("SIprocess index=%d>maxid=%d\n",index,maxid);
                   int maxretry=(index-maxid)<20?2:((index-maxid)<200?5:10);
                   if(sens->retried++<maxretry) {
                      return 3LL;
                      }
                   }
               }
        if(maxid<10) {
               const auto starttime=makestarttime(index,eventTime);
               sens->getinfo()->starttime=starttime;
               sensor->starttime=starttime;
               sensors->setindices();
               backup->resendResetDevices(&updateone::sendstream);
               }
        double temp=basear[1]/10.0;
        auto current= basear[2];
        double value=current/10.0;
        LOGGER("current=%" PRId64 " %.1f mmol/L\n",current,value);
        int reindex=(int)basear[4];
        auto trend=(int)basear[6];

       double newvalue;
       if(algcontext) {
           if(current>1&&value<3000.0&&(newvalue= process2(index,value,temp))>1.8&&(index%5==0)) {
                sens->getinfo()->pollinterval=newvalue-value;
                }
           else {
             if(sens->getinfo()->pollinterval<40) 
                    newvalue=value+sens->getinfo()->pollinterval;
             }
             }
       else   {
           LOGAR("algcontext==null");
            newvalue=value;
            }
           const int mgdL=std::round(newvalue*convfactordL);
           const int trend2=algcontext?algcontext->ig_trend:trend;
           const float change=sitrend2RateOfChange(trend2);
           const int abbottrend=sitrend2abbott(trend2);
           const int totalIndex=sens->siAddedIndex(index);
           LOGGER("totalIndex=%d index=%d temp=%f value=%f newvalue=%f trend=(%d?) %d %d %1.f itime=%" PRIu64 " %s" ,totalIndex,index,temp,value,newvalue,trend,trend2,abbottrend,change,eventTime,ctime(&eventTime));
          if(newvalue>1.8&&newvalue<30) {
               sens->savestream(eventTime,totalIndex,mgdL,abbottrend,change);
               sens->setSiIndex(index+1);
               sens->retried=0;
               if(!reindex)  {
                    uint32_t starttime=sens->getinfo()->starttime;
                    uint32_t warminutes=sens->getinfo()->manualwarmup;
                    uint32_t endwarmup=starttime+warminutes*60;
                    if(eventTime>endwarmup) {
                         sens->sensorerror=false;
                         if(sensor->finished) {
                                sensor->finished=0;
                                LOGGER("SIprocess finished=%d\n", sensor->finished);
                                backup->resensordata(sensorindex);
                                }
                         auto res=glucoseback(eventTime,mgdL,change,sens);
                         backup->wakebackup(wakestream);
                         extern void wakewithcurrent();
                         wakewithcurrent();

        #ifdef OLDEVERSENSE
                          sendEverSenseold(sens,5);
        #endif
                           return res;
                          }
                      else {
                            if(!sens->getinfo()->redoAll) {
                                sens->getinfo()->warmupstartpos=std::min((int)sens->getinfo()->pollcount-1,0);
                                }
                           
                          }
                         }
               else {
                     sens->receivehistory=nowsecs;
                     }
               const int last=sens->pollcount()-1;
               if(last<sens->getbroadcastfrom()) sens->setbroadcastfrom(last);
               }
        else {
                if(index==maxid) sens->setSiIndex(maxid+1);
                LOGGER("SIprocess failed: index=%d temp=%f value=%f reindex=%d\n", index, temp, value,reindex);
                if(!reindex&&!(index%5))  {
                      sens->sensorerror=true;
                      sens->sensorErrorTime=nowsecs;
                      return 0LL;
                      }
                    }
                 basear+=11;
                }//for loop
             return 1LL;
        };break;
    case 0xC00D: { //49165: 
        int type=(int) basear[0];
        switch(type) {
           case 0xC009 /*49161*/: return 9LL;
           case 0xC004 /*49156*/: return 7LL;
           case 0xC001 /*49153*/: return 4LL;
           case 0xC002 /*49154*/:  {
                   if(basear[1]!=1) {
                      int error=(int)basear[2];
                      if(error!=9&&error!=10) return 4LL; 
                      }
                    return 6LL;
                    };
            case 0xC008 /*49160*/: return 5LL;
           };
        
        };break;
    case 0xC04B /*49227*/: {
            if(sens->siSubtype()==3)  {
                return 10LL;
                }
            return 6LL; //Never used??
            }
       }
      return 8LL; ///?
    }


#else
#include <jni.h>
#include "fromjava.h"
extern "C" JNIEXPORT jbyteArray JNICALL   fromjava(getSIActivation)(JNIEnv *env, jclass cl) {
   return nullptr;
   }
extern "C" JNIEXPORT jbyteArray JNICALL   fromjava(getSItimecmd)(JNIEnv *env, jclass cl) {
   return nullptr;
   }
extern "C" JNIEXPORT jbyteArray JNICALL   fromjava(siAuthBytes)(JNIEnv *env, jclass cl,jlong dataptr) {
   return nullptr;
   }
#endif
#endif
