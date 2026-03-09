#include "logs.hpp"
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include <charconv>
#include "air.hpp"
template <typename T>
char *  numberstring(char *ptr,char *end,T one) {
        auto res=std::to_chars(ptr,end,one); 
        *res.ptr='\0';
        return res.ptr;
        }
template <typename T,typename ... Ts>
     char *   numberstring(char *ptrin,char *end,T one,Ts...args) {
        auto res=std::to_chars(ptrin,end,one); 
        char *ptr=res.ptr;
        *ptr++=',';
        *ptr++=' ';
        return numberstring(ptr,end,args...);
        }
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
void showm(int index,const  m *mptr,const m *end ) {

        for(;mptr<end;++mptr) {
            time_t time=mptr->measurement_time;
            constexpr int maxbuf=400;
            char buf[maxbuf];
            char *endbuf=buf+maxbuf;
            int endpos=snprintf(buf,maxbuf,"%d m seq_num %d temperature %.1f unixtime=%lu: ",index, mptr->sequence_number,mptr->temperature,time);
            char *end=printarray(buf+endpos,endbuf,mptr->glucose_array.data(),30);
            *end++=' ';
            ctime_r(&time,end);
            LOGGERN(buf,end-buf+24);
            }
          }
bool showsizes() {
        LOGGER("sizeof(DeviceInfo3Obj)=%zd\n",sizeof(DeviceInfo3Obj)) ;
        LOGGER("sizeof(DeviceInfo2Obj)=%zd\n",sizeof(DeviceInfo2Obj)) ;
        LOGGER("sizeof(m)=%zd\n",sizeof(m)) ;
        LOGGER("sizeof(j)=%zd\n",sizeof(j)) ;
        LOGGER("sizeof(g0)=%zd\n",sizeof(g0)) ;
        return true;
        }
void   showRUNar(int index,const uint8_t *bytes,int len) {
        static bool shown=showsizes();
        if(len==117312) {
                LOGGER("showRUNar %d len==%d generated\n",index,len);
                return;
                }
        if(sizeof(DeviceInfo3Obj)==len) { 
                const auto *devinfo=reinterpret_cast<const DeviceInfo3Obj*>(bytes);
                time_t starttime=devinfo->sensor_start_time;
                LOGGER("%d: DeviceInfo3Obj sensor_id %.12s %s",index,devinfo->sensor_id,ctime(&starttime));
                return;
                }
        if(sizeof(DeviceInfo2Obj)==len) { 
                const auto *devinfo=reinterpret_cast<const DeviceInfo2Obj*>(bytes);
                LOGGER("%d: DeviceInfo2Obj sensor_id %.12s",index,devinfo->sensor_id);
                return;
                }
        if(len%sizeof(m)==0) { 
                const auto *end=reinterpret_cast<const m*>(bytes+len);
                LOGGER("showRUNar size=%d->m\n",len);
                const auto *mptr=reinterpret_cast<const m*>(bytes);
                showm(index,mptr,end);
                return;
                }
        if(len%sizeof(g0)==0) { 
                LOGGER("showRUNar size=%d->g0\n",len);
                const auto *endptr=reinterpret_cast<const g0*>(bytes+len);
                for(const auto *mptr=reinterpret_cast<const g0*>(bytes);mptr<endptr;++mptr) {
                    LOGGER("mptr=%p\n",mptr);
                    time_t time=mptr->measurementTimeSecs;
                    constexpr int maxbuf=650;
                    char buf[maxbuf];
                    char *endbuf=buf+maxbuf;
                    double mgdL=mptr->initialValue;
                    double mmolL=mgdL/18.0;
                    int endpos=snprintf(buf,maxbuf,"%d g0 error=%d seq_num %d bleIndex=%d value %.2f mg/dL %.2f mmol/L trendRate=%.2f unixtime=%lu: ",index,mptr->errorCode,mptr->sequenceNumber,mptr->bleIndex,mgdL,mmolL,mptr->trendRate,time);
                    char *end=printarray(buf+endpos,endbuf,mptr->glucose_array.data(),30);
                    *end++=' ';
                    ctime_r(&time,end);
                    char *ptr=end+24;
                    for(int i=0;i<6;++i) {
                        auto mgdL= mptr->valuesArray[i];
                        auto mmolL=mgdL/18.0;
                        ptr+=snprintf(ptr,ptr-buf,"\nseq_num %d value %.2f mg/dL %.2f mmol/L %d",mptr->sequenceNumberArray[i],mgdL,mmolL ,mptr->nums[i]);

                        }
                    LOGGERN(buf,ptr-buf);
                    }
                return;
                }
        if(len%sizeof(n)==0) { 
                LOGGER("showRUNar size=%d->n\n",len);
                const auto *end=reinterpret_cast<const n*>(bytes+len);
                for(const auto *mptr=reinterpret_cast<const n*>(bytes);mptr<end;++mptr) {
                    time_t time=mptr->measurement_time;
                    constexpr int maxbuf=400;
                    char buf[maxbuf];
                    char *endbuf=buf+maxbuf;
                    int endpos=snprintf(buf,maxbuf,"%d n seq_num %d unixtime=%lu: ",index,mptr->sequence_number,time);
                    char *end=printarray(buf+endpos,endbuf,mptr->glucose_array.data(),30);
                    *end++=' ';
                    ctime_r(&time,end);
                    LOGGERN(buf,end-buf+24);
                    }
                return;
                }
        if(len%sizeof(j)==0) { 
                LOGGER("showRUNar size=%d->j:\n",len);
                const j *end=reinterpret_cast<const j*>(bytes+len);
                for(const j *jptr=reinterpret_cast<const j*>(bytes);jptr<end;++jptr) {
                    time_t time=jptr->measurement_time_sec;
                    LOGGER("%d j idx=%hd glucose=%.1f %lu %s",index,jptr->idx,jptr->glucose_value,time,ctime(&time));
                    }
                return;
                }

        LOGGER("%d showRUNar size=%d-> nothing fits\n",index,len);
        }

