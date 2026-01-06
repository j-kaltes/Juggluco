/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2, Libre 3, Dexcom G7/ONE+ and           */
/*      Sibionics GS1Sb sensors.                                                     */
/*                                                                                   */
/*      Copyright (C) 2021 Jaap Korthals Altes <jaapkorthalsaltes@gmail.com>         */
/*                                                                                   */
/*      Juggluco is free software: you can redistribute it and/or modify             */
/*      it under the terms of the GNU General Public License as published            */
/*      by the Free Software Foundation, either version 3 of the License, or         */
/*      (at your option) any later version.                                          */
/*                                                                                   */
/*      Juggluco is distributed in the hope that it will be useful, but              */
/*      WITHOUT ANY WARRANTY; without even the implied warranty of                   */
/*      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                         */
/*      See the GNU General Public License for more details.                         */
/*                                                                                   */
/*      You should have received a copy of the GNU General Public License            */
/*      along with Juggluco. If not, see <https://www.gnu.org/licenses/>.            */
/*                                                                                   */
/*      Sat Jul 05 15:19:21 CEST 2025                                                */


#include <vector>
#include <math.h>
#include <algorithm>
#include <numeric>
#include "secs.h"
#include "SensorGlucoseData.hpp"
#include "nums/numdata.hpp"
#include "sensoren.hpp"
#include "calibrateValue.hpp"
constexpr const int maxdifference=3*60;
extern bool shouldexclude(const uint32_t time) ;
extern vector<Numdata*> numdatas;
template <typename DT> const DT *getdata(const SensorGlucoseData *sens,int pos);
template<>
const ScanData *getdata<const ScanData>(const SensorGlucoseData *sens,int pos) {
    return sens->beginpolls()+pos;
    }
template<>
const ScanData *getdata<ScanData>(const SensorGlucoseData *sens,int pos) {
    return sens->beginpolls()+pos;
    }
template<>
const Glucose *getdata<Glucose>(const SensorGlucoseData *sens,int pos) {
    return sens->getglucose(pos);
    }
template<>
const Glucose *getdata<const Glucose>(const SensorGlucoseData *sens,int pos) {
    return sens->getglucose(pos);
    }

template <typename DT> const char *givename();
template <> const char *givename<ScanData>(){
    return "Stream";
    }
template <> const char *givename<Glucose>(){
    return "History";
    }
#include "calculate.hpp"

/*
static constexpr const double  maxCalSecs=14*daysecs;
static double mkweight(double age) {
    return (maxCalSecs-age)/maxCalSecs; 
    } */


#include "Calibrator.hpp"

template <typename DT>
static  const DT *firstnotless(const DT *scan, const DT *endscan,const uint32_t tim) {
    LOGGER("firstnotless from %u to %u toget %u\n",scan->gettime(),(endscan-1)->gettime(),tim);
    const DT scanst{.t=tim};
    auto comp=[](const DT &el,const DT &se ){return el.t<se.t;};
    const DT *hit=std::lower_bound(scan,endscan, scanst,comp);
    while(hit<endscan&&!hit->valid())  {
        ++hit;
        }
    return hit;
    }
template <typename DT>
  std::pair<int,const DT*> firstnotless(const SensorGlucoseData *sens,int first, int last,const uint32_t tim);
template <>
 std::pair<int,const ScanData*> firstnotless<ScanData>(const SensorGlucoseData *sens,int first, int last,const uint32_t tim) {
        auto *start=getdata<ScanData>(sens,first);
        const ScanData *data=firstnotless(start, getdata<ScanData>(sens,last),tim);
        int pos=data-start+first;
        LOGGER("Stream: firstnotless(%d,%d,%u)=%d %u\n",first,last,tim,pos,data->gettime());
        return {pos,data};
    }
template <>
 std::pair<int,const Glucose*> firstnotless<Glucose>(const SensorGlucoseData *sens,int first, int last,const uint32_t tim) {
        int pos=sens->getfirstnotbeforetime(tim);
        if(pos>=last) {
            return {last,nullptr};
            }
        if(pos<first)
            pos=first;
        const Glucose *glu=sens->getglucose(pos);
        return {pos,glu};
    } 
extern std::mutex caliMutex;
std::mutex caliMutex;
extern void setCalibrates(uint16_t sensorindex) ;
extern void removeCalibration(const Num *num);


static void removeCalibrationThread(const uint32_t tim) {
    vector<int> sensinds=sensors->sensorsInPeriod(tim-5*60, tim+5*60);
    if(sensinds.size()) {
        for(int index:sensinds) {
            SensorGlucoseData *sens=sensors->getSensorData(index);
            {
                const std::lock_guard<std::mutex> lock(caliMutex);
                auto calibrate = make_calibrator<ScanData>(sens);
                int rempos=calibrate.removeCali(tim);
                calibrate.reCalcCali(rempos);
                if(sens->hasHistory()) {
                    auto calibrate = make_calibrator<Glucose>(sens);
                    int rempos=calibrate.removeCali(tim);
                    calibrate.reCalcCali(rempos);
                    }
            }
            }
        setCalibrates(*std::ranges::min_element(sensinds));
        }
    }
void removeCalibration(uint32_t tim) {
    std::thread remthread(removeCalibrationThread,tim);
    remthread.detach();
    }
void removeCalibration(const Num *num) {
    if(num->type!=settings->data()->bloodvar)
        return;
    removeCalibration(num->gettime());
     }

template <typename DT>
static bool wrongChange(const DT *value) {
        return false;
        }
template <ScanData>
static bool wrongChange(const ScanData *value) {
        if(value->getchange()>=1.0) {
            #ifndef NOLOG
            time_t tim=value->gettime();
            LOGGER("Rises too much %.1f %s",value->getchange(),ctime(&tim));
            #endif
            return true;
            }
        if(value->getchange()<=-1.0) {
            #ifndef NOLOG
            time_t tim=value->gettime();
            LOGGER("falls too much %.1f %s",value->getchange(),ctime(&tim));
            #endif
            return true;
            }
       return false;
    } 
static int maxneighbourtimedifference=6*60;
//static int maxglucosedifference=9;
//Should also have a smaller influenc when calibrated with a larger value
/*static constexpr const float getMaxglucosedifference(float val) {
    return 0.0272727f*(val - 70.0f) + 9.0f;
    } */
static constexpr const auto getMaxglucosedifference(auto val) {
    return 10;
    }

template <typename DT>
static bool         wrongNeighbours(const SensorGlucoseData *sens,int startsen,int endsen,int valuepos,DT *value) {
    const uint32_t startInterval=value->gettime()-maxneighbourtimedifference;
    const auto theval=value->getmgdL();
    const auto maxglucosedifference=getMaxglucosedifference(theval);
    const auto mgdLmin=theval-maxglucosedifference;
    const auto mgdLmax=theval+maxglucosedifference;
    for(int itpos=valuepos-1;itpos>=startsen;--itpos) {
        auto it=getdata<DT>(sens,itpos);
        if(it->gettime()<=startInterval)
            break;
        const auto val=it->getmgdL();
        if(val<mgdLmin||val>mgdLmax) {
    #ifndef NOLOG
           time_t tim=time(nullptr);
            LOGGER("wrongNeighbours:  %d extreme %s",val,ctime(&tim));
    #endif
            return true;
            }
        }
    const uint32_t endInterval=value->gettime()+maxneighbourtimedifference;
    for(int itpos=valuepos+1;itpos<endsen;++itpos) {
        auto it=getdata<DT>(sens,itpos);
            if(it->gettime()>=endInterval)
                break;
        const auto val=it->getmgdL();
        if(val<mgdLmin||val>mgdLmax) {
            LOGGER("wrongNeighbours too large difference between %d and %d diff=%d maxdifference %d\n",val,theval,abs((int)(theval-val)),maxglucosedifference);
            return true;
            }
        }
    return false;
    }

template <typename DT> int getinterval(const SensorGlucoseData *sens);
template<> int getinterval<Glucose>(const SensorGlucoseData *sens) {
    return sens->getinterval();
    }
template<> int getinterval<ScanData>(const SensorGlucoseData *sens) {
    return sens->getsecstreaminterval();
    }
template <typename DT>
static bool notSuitable(const SensorGlucoseData *sens,uint32_t numtim,int startsen,int endsen,int valuepos,const DT *value,int maxdifference) {
        const int64_t sensLater=((int64_t)value->gettime())-numtim;
        if(sensLater>maxdifference) {
                LOGGER("too late sensor %u, blood %d %lld\n",value->gettime(),numtim,sensLater);
                return true;
                }
        if(wrongChange(value)) return true;
        return wrongNeighbours(sens,startsen,endsen,valuepos,value);
        }

template <typename DT>
std::pair<int,const DT*> findNextCGM(const SensorGlucoseData *sens,int startsen,int endsen,uint32_t numtim,int maxdiff) {
        if(startsen==endsen) {
            LOGAR("findNextCGM startsen==endsen");
            return {-2,nullptr};
            }
        auto [posafter,after]=firstnotless<DT>(sens,startsen,endsen,numtim);
        if(posafter>=endsen)  {
            do{
              --posafter;
              if(posafter<=startsen) {
                    LOGGER("findNextCGM no value around %u\n",numtim);
                    return {-2,nullptr};
                    }
              after=getdata<DT>(sens,posafter);
              }while(!after->valid());
            LOGGER("posafter=%d time=%u\n",posafter,after->gettime());
            }
        if((after->gettime()+maxdifference)<numtim) {
    #ifndef NOLOG
            time_t tim=after->gettime();
            LOGGER("findNextCGM: last value too early  %s",ctime(&tim));
    #endif
            return {-2,nullptr};
            }
        if(notSuitable<DT>(sens,numtim,startsen,endsen,posafter,after,maxdiff)) {
            return {-1,nullptr};
            }
        #ifndef NOLOG
         time_t tim=after->gettime();
        LOGGER("findNextCGM: good value %.1f %s",after->getmmolL(),ctime(&tim));
        #endif
         return {posafter,after};
         }


template<typename DT> int getfirstpos(const SensorGlucoseData *sens);
template<>
int getfirstpos<ScanData>(const SensorGlucoseData *sens) {
    return sens->getinfo()->pollstart;
    }
template<typename DT> int getlastpos(const SensorGlucoseData *sens);
template<>
int getlastpos<ScanData>(const SensorGlucoseData *sens) {
    return sens->pollcount();
    }

template<>
int getfirstpos<Glucose>(const SensorGlucoseData *sens) {
    return sens->getstarthistory();
    }
template<>
int getlastpos<Glucose>(const SensorGlucoseData *sens) {
    return sens->getAllendhistory();
    }

template <typename DT>
int shouldexclude(const uint32_t time) {
   uint32_t starttime=time-maxdifference;
   uint32_t endtime=time+maxdifference;
   vector<int> indices=sensors->sensorsInPeriod(starttime, endtime);
   if(!indices.size())  {
        LOGGER("shouldexclude: no sensors between %u and %u\n",starttime,endtime);
        return true;
        }
   bool hasValue=false;
    for(int index:indices) {
        auto *sens=sensors->getSensorData(index);
       const int maxdiff=std::max(getinterval<DT>(sens),3*60);
        int firstpos=getfirstpos<DT>(sens);
        int lastpos=getlastpos<DT>(sens);
        if(lastpos>firstpos) {
            const auto [posel,el]=findNextCGM<DT>(sens,firstpos,lastpos,time,maxdiff);
            if(posel>-2) {
                hasValue=true;
                if(el) {
                    const int mgdL=el->getmgdL();
                    if(mgdL>=sens->getminmgdL()&&mgdL<=sens->getmaxmgdL())  {
                        LOGGER("shouldexclude:  %u - %u nrsensors=%d don't exclude\n",starttime,endtime,indices.size());
                        return false;
                        }
                     else {
                        LOGGER("shouldexclude: %d out of range\n",mgdL);
                        }
                    }
               }
          }

        }
    if(hasValue) {
          LOGGER("shouldexclude:  %u - %u nrsensors=%d wrong\n",starttime,endtime,indices.size());
          return -1;
          }
    LOGGER("shouldexclude:  %u - %u nrsensors=%d nodata\n",starttime,endtime,indices.size());
     return -2;
    }

template  int shouldexclude<ScanData>(const uint32_t time);


bool shouldexclude(const uint32_t tim) {
    if(int res=shouldexclude<ScanData>(tim))  {
      int res2;
        if(res>0||((res2=shouldexclude<Glucose>(tim))&&res!=-2&&res!=-2)) {
                return true;
              } 
        }
    return false;
    }
template <typename DT>  uint32_t getfirsttime(const SensorGlucoseData *sens);
template <> 
 uint32_t    getfirsttime<ScanData>(const SensorGlucoseData *sens) {
       return sens->firstpolltime();
    }
template <> 
uint32_t getfirsttime<Glucose>(const SensorGlucoseData *sens) {
       return sens->getfirsttimehistory();
    }
template <typename DT> CalcPara calculate(const SensorGlucoseData *sens, const uint32_t newtime) {
   const int maxdiff=std::max(getinterval<DT>(sens),3*60);
 #ifndef NOLOG
    const char *name=givename<DT>();
#endif
    const auto oldtime=getfirsttime<DT>(sens);
    const int bloodvar=settings->data()->bloodvar;
    std::vector<double> y,x,w;
    int startsen=getfirstpos<DT>(sens);
    int endsen=getlastpos<DT>(sens);
    const int mindistance=30*60;
    uint32_t nexttime=UINT_MAX;
    const int minmgdL=sens->getminmgdL();
    const int maxmgdL=sens->getmaxmgdL();
    bool firstcalibration=true;
    for(const Numdata *numdata:numdatas) {
        LOGGER("%s calculate firstpos=%d endpos=%d\n",name,startsen,endsen);
        const Num* endnum=numdata->firstAfter(newtime);
        const Num* begnum=numdata->begin();
        for(const Num *num=endnum-1;num>=begnum;--num) {
            if(numdata->valid(num)) {
                const uint32_t numtim=num->gettime();
                if(numtim<oldtime) {    
                    break;
                    }
                if(numtim>nexttime) {
                    #ifndef NOLOG
                    if(num->type==bloodvar) {
                        LOGGER("%s: calibrate: %u too near previousvalue\n",name,numtim);
                        }
                    #endif
                    continue;
                    }
                if(!num->calibrator(bloodvar))
                    continue;
                auto [posafter,after]=findNextCGM<DT>(sens,startsen,endsen,numtim,maxdiff);
                if(posafter<0)
                    continue;
                double weight=mkweight(newtime-numtim);
                if(weight<=0.0)
                    continue;
                uint32_t timeafter=after->gettime(); 
                const double mgdLafter= after->getmgdL();
                if(mgdLafter<=minmgdL) {
                    LOGGER("%s: calibrate calculate %d too low\n",name,mgdLafter);
                    continue;
                    }
                if(mgdLafter>=maxmgdL) {
                    LOGGER("%s: calibrate calculate %d too high\n",name,mgdLafter);
                    continue;
                    }
                double streamvalue;
                nexttime=numtim-mindistance;
                if(timeafter>numtim) { 
                    int posbefore=posafter-1;
                    if(posbefore<0)
                        continue;
                    auto before=getdata<DT>(sens,posbefore);
                    if(!before->valid())
                        continue;
                    uint32_t timebefore=before->gettime(); 
                    int diff=(numtim-timebefore);
                    if((numtim-timebefore)>maxdiff) {
                        LOGGER("%s: too far before %u numtime %u diff=%d maxdiff=%d\n",name,timebefore,numtim,diff,maxdiff);
                        continue;
                        }
                    const double mgdLbefore=before->getmgdL();
                    if(mgdLbefore<=minmgdL) {
                        LOGGER("%s: calibrate calculate %d too low\n",name,mgdLbefore);
                        continue;
                        }
                    if(mgdLbefore>=maxmgdL) {
                        LOGGER("%s: calibrate calculate %d too high\n",name,mgdLbefore);
                        continue;
                        }
                    streamvalue=mgdLbefore + ((mgdLafter - mgdLbefore)*(numtim - timebefore))/(timeafter - timebefore);
                    endsen=posbefore;
                    }
                else {
                    endsen=posafter;
                    streamvalue=mgdLafter;
                    }
                #ifndef NOLOG
                char buf1[27],buf2[27];
                time_t tnumtim=numtim;
                time_t tsenstim=after->gettime();
                #endif
                if(firstcalibration) {
                        if(numtim<newtime) {
                                constexpr const double nan=NAN;
                                return {nan,nan,(float)NAN};
                                }
                        firstcalibration=false; 
                        }
                double bloodmgdL=backconvert(num->value)*.1f;
                y.push_back(bloodmgdL);
                //double reweight=1.04348 - 0.00108696 *bloodmgdL;
                //double reweight= 0.160911 *log(540 - bloodmgdL); //Less weigth for high glucose values
                //double reweight=1/(1 + pow(log(1 + exp(1/80 (g-140))),2));
                double reweight= 1.0/(1.0 + 0.591716 * pow(log(0.0536647* (18.6342 + exp(0.01625 *bloodmgdL))),2)); //  =1/(1 + pow(log(1 + exp(1/80 (g-140))),2));
                double reweighted=weight*reweight;

                w.push_back(reweighted);
                x.push_back(streamvalue);
                LOGGER("%s: calculate: num %.1f %.24s stream %.1f weight=%f reweight=%f reweighted=%f %s\n",name,num->value,ctime_r(&tnumtim,buf1),gconvert(streamvalue*10),weight,reweight,reweighted,ctime_r(&tsenstim,buf2));

                }
            }
          }
#ifndef NOLOG
    printvector(name,"x",x);
    printvector(name,"y",y);
    printvector(name,"w",w);
#endif
    int nr=x.size();
    if(nr<1) {
        constexpr const double nan=NAN;
        return {nan,nan,(float)NAN};
        }
    const long double totweight=std::reduce(std::begin(w),std::end(w),(long double){});
    if(!settings->data()->DoNotCalibrateA) {

      /*  const auto [meanstream,count]=mean_mgdL(stream);
        const double sdstream=sd_mgdL(meanstream,count,stream)
        const auto [meancali,countcali]=mean_mgdL(x);
        const double sdcali=sd_mgdL(meancali,countcali,x);*/ //not devided by count-1. Data should be of the same kind
        //LOGGER("sd sensor=%.2f sd calibratie=%.2f\n",sdstream,sdcali);
        if(totweight<5.0||sd_mgdL(x)<=sd_mgdL<const SensorGlucoseData,DT>(sens)*.7) {
            y.push_back(0); 
            w.push_back(1); 
            x.push_back(0); 
            ++nr;
            }
        double preA=getA(w,x,y,nr);
        double a=moderateA(preA,totweight,2.0);
        double preB=getB(w,x,y,nr);
        double b=moderateB(preB,totweight,2.0);
        LOGGER("calibrate: preA=%.2f a=%.2f preB=%.2f b=%.2f\n",preA,a,preB,b);
        return {a,b,(float)totweight}; 
        }
     double preB=distance(w,x,y, nr)/totweight;
     double b =moderateB(preB,totweight,2.0);
     LOGGER("calibrate: preB=%.2f b=%.2f\n",preB,b);
     return {1.0,b,(float)totweight};
    }

template  CalcPara calculate<ScanData>(const SensorGlucoseData *sens, const uint32_t newtime);
template  CalcPara calculate<Glucose>(const SensorGlucoseData *sens, const uint32_t newtime);
   /* 
struct HistCalibrator:Calibrator {
    CalcPara calculate(const uint32_t newtime) {
            return calculate(sens->getPolldata(),sens->firstpolltime(),sens, newtime);
            }
    }; */
template <typename DT,typename Sens,typename Cali>
 void Calibrator<DT,Sens,Cali>::reCalcCali( int fromnr) {
    int caliNr=cali.caliNr;
    if(fromnr>=caliNr) {
        return;
        } 
    LOGGER("reCalcCali %s %d-%d\n",sens->shortsensorname()->data(),fromnr,caliNr);
    auto *caliPara=cali.caliPara;
    int uititer=fromnr;
    for(int it=fromnr;it<caliNr;++it) {
        const auto [a,b,weight]=calculate(caliPara[it].time);
        if(isnan(a)) {
            LOGGER("reCalcCali %d a is nan\n",it);
            continue;
            }
        if(isnan(b)) {
            LOGGER("reCalcCali %d b is nan\n",it);
            continue;
            }
        caliPara[uititer].time=caliPara[it].time;
        caliPara[uititer].weight=weight;
        caliPara[uititer].a=a;
        caliPara[uititer].b=b;
        ++uititer;
        }
    cali.caliNr=uititer;
    }
template <typename DT,typename Sens,typename Cali>
 bool Calibrator<DT,Sens,Cali>::changeCali(const uint32_t oldtime,const uint32_t newtime,const Num *num, const Numdata *numdata) {
 #ifndef NOLOG
    const char *name=givename<DT>();
#endif
    int rempos=-1;
    if(oldtime)
        rempos=cali.removeCali(oldtime);
    const auto [a,b,weight]=calculate( newtime);
    if(isnan(a)) {
        LOGGER("%s: changeCali a is nan\n",name);
        if(rempos>=0)
            reCalcCali(rempos);
        return false;
        }
    if(isnan(b)) {
        LOGGER("%s: changeCali b is nan\n",name);
        if(rempos>=0)
            reCalcCali(rempos);
        return false;
        }
      {
    const std::lock_guard<std::mutex> lock(caliMutex);
    if(num<numdata->end()&&num->gettime()==newtime) {
        int calpos=cali.addCali(newtime,weight,a,b);
        if(rempos>=0&&oldtime<=newtime)
            reCalcCali(rempos);
         else {
            if(calpos>=0)
                reCalcCali(calpos+1);
            }
        return true;
        }
   else {
       LOGGER("name %s: changeCali %u not longer present\n",name,newtime);
        if(rempos>=0)
            reCalcCali(rempos);
        return false;
        }
      }
    }

static std::pair<int,bool> calibrateIndices2(const vector<int> &sens,uint32_t oldtime,uint32_t tim,const Num *num,const Numdata *numdata) {
        int minwait=0;
        bool allsuccess=true;
        for(int index:sens) {
            LOGGER("calibrateIndices2 %d\n",index);
            auto *sensor=sensors->getSensorData(index);
            allsuccess&=make_calibrator<ScanData>(sensor).changeCali(oldtime,tim,num,numdata);
            int waithere;
            if(sensor->hasHistory()) {
                allsuccess&=make_calibrator<Glucose>(sensor).changeCali(oldtime,tim,num,numdata);
                waithere=sensor->isAir()?31:21;
                }
            else
                waithere=5;
            if(waithere>minwait)
                minwait=waithere;
            }
        setCalibrates(*std::ranges::min_element(sens));
        extern void render(); 
        render(); 
        backup->wakebackup(wakenums);
        return {minwait*60,allsuccess};
    }
static void calibrateIndices(const vector<int> &sens,uint32_t oldtime,uint32_t tim,const Num *num,const Numdata *numdata) {
    const auto [minwait,_]=calibrateIndices2(sens,oldtime,tim,num,numdata);
    const uint32_t now=time(nullptr);
    const int  ago=now-tim;
    LOGGER("calibrateIndices now=%u calibration time=%u ago=%d minwait=%d\n",now,tim,ago,minwait);
      if(ago<minwait) {
            std::this_thread::sleep_for(std::chrono::seconds(minwait-ago));
            LOGAR("calibrateIndices after sleep");
            calibrateIndices2(sens, oldtime, tim,num,numdata);
           }
    }
static void threadCalibration(uint32_t oldtime,uint32_t tim,const Num *num,const Numdata *numdata) {
    vector<int> sens=sensors->sensorsInPeriod(tim-5*60, tim+5*60);
    if(sens.size()) {
        #ifndef HAVE_NOPRCTL
              prctl(PR_SET_NAME, "Cali", 0, 0, 0);
        #endif
        LOGGER("threadCalibration: %d sensors\n",sens.size());
        calibrateIndices(sens, oldtime, tim,num,numdata);
        }
    else {
        LOGGER("threadCalibration: no sensors at %u\n",tim);

        }
     
    }
extern Numdata *getherenums();
static void calibrateLastThread() {
    const int bloodvar=settings->data()->bloodvar;
    time_t now=time(nullptr);
#ifndef NOLOG
    vector<int> sensindices=sensors->sensorsInPeriod(now-60*24*60*60, now);
#else
    vector<int> sensindices=sensors->sensorsInPeriod(now-60*60, now);
#endif
    if(sensindices.size()<=0) {
        LOGAR("calibrateLast  no sensors");
        return;
        }
    
#ifndef HAVE_NOPRCTL
      prctl(PR_SET_NAME, "CaliLast", 0, 0, 0);
#endif
    uint32_t previoustime=0;
    for(auto index:sensindices) {
        LOGGER("calibrateLastThread sensor %d\n",index);
        const auto *calis=sensors->getSensorData(index)->getinfo()->calis;
        auto tim=calis[0].lastCalibrated();
        if(tim>previoustime)
            previoustime=tim;
        }
    const Numdata *numdata=getherenums();
    while(true) { 
        const Num *start=numdata->begin();
        const Num *ends=numdata->end();
        int allminwait=0;
        uint32_t allwastime;
        for(const Num*it=ends-1;it>=start;--it) {
            if(!numdata->valid(it))
                continue;
            if(it->gettime()<previoustime) {
                LOGGER("calibrateLast %u before previoustime %u\n",it->gettime(),previoustime);
                return;
                }
            if(it->calibrator(bloodvar)) {
                uint32_t wastime=it->gettime();
                const auto [minwait,success]=calibrateIndices2(sensindices, 0, wastime,it,numdata);
                if(minwait>allminwait)  {
                    allminwait=minwait;
                    allwastime=wastime;
                    }
                if(success)
                    break;
                }
            }
        if(allminwait>0) {
            uint32_t now=time(nullptr);
            const int ago=now-allwastime;
            LOGGER("calibrateLastThread now=%u calibration time=%u ago=%d minwait=%d\n",now,allwastime,ago,allminwait);
            if(ago<allminwait) {
                std::this_thread::sleep_for(std::chrono::seconds(allminwait-ago));
                LOGAR("calibrateLastThread after sleep");
                continue;
                }
             }
        else {
            LOGAR("calibrateLast no not excluded blood measurement");
            }
         break;
         };
    }

void calibrateLast() {
    if(settings->data()->bloodvar>=maxvarnr) {
        LOGAR("calibrateLast bloodvar not set");
        return;
        }
    std::thread  th(calibrateLastThread);
    th.detach();
    }
extern void addCalibration(uint32_t tim,int type,Num *num,const Numdata *numdata) ;
void addCalibration(uint32_t tim,int type,Num *num,const Numdata *numdata) {
    if(type!=settings->data()->bloodvar)
        return;
    if(num->exclude) {
        LOGGER("addCalibration exclude %u\n",tim);
        return;
        }
    if(shouldexclude(tim))  {
        LOGGER("addCalibration %u set exclude=true\n",tim);
        num->exclude=true;
        return;
        }
    std::thread  th(threadCalibration,0u,tim,num,numdata);
    th.detach();
    }

void changeCalibration(uint32_t oldtime,bool oldexclude,uint32_t tim,int type,Num *num,const Numdata *numdata) {
    if(type!=settings->data()->bloodvar)
        return;
    if(num->exclude) {
        if(!oldexclude)
            removeCalibration(oldtime);
        LOGGER("addCalibration exclude %u\n",tim);
        return;
        }

    if(shouldexclude(tim))  {
            if(!oldexclude)
                removeCalibration(oldtime);
            LOGGER("addCalibration %u set exclude=true\n",tim);
            num->exclude=true;
            return;
        }
    if(oldexclude)
        oldtime=0;
    std::thread  th(threadCalibration,oldtime,tim,num,numdata);
    th.detach();
    }
template <typename T>
double calibrateValue(const CaliPara &cali ,T&el) {
       return calibrateValue(cali , el.gettime(),el.getmgdL());
    }



const CaliPara *getCaliBefore(const CaliPara *first,const CaliPara *end,uint32_t time) {
    CaliPara zoek;
    zoek.time=time;
    const CaliPara *cali=std::lower_bound(first,end,zoek,[](const CaliPara &one,const CaliPara &two) {
            return one.time<two.time;
            });
    if(cali==first) {
        return nullptr;
        }
    return cali-1;
    }

template <typename DT,typename Sens,typename Cali>
double     Calibrator<DT,Sens,Cali>::calibrateONE(const uint32_t time, const double value) {
    const uint32_t nr=cali.caliNr;
    if(!nr)  {
        LOGGER("calibrateONE(%s,%u,%.1f) no calibrators\n",sens->shortsensorname()->data(),time,value);
        return NAN;
        }
    const CaliPara *first = cali.caliPara;
    if(settings->data()->CalibratePast) 
        return calibrateValue(first[nr-1],time,value);
        
    if(const CaliPara *cali=getCaliBefore( first,first+nr,time)) {
        return calibrateValue(*cali,time,value);
        }
    LOGGER("calibrateONE(%s,%u,%.1f) no calibrator before time\n",sens->shortsensorname()->data(),time,value);
    return NAN;
    }
template double    Calibrator<ScanData, SensorGlucoseData, Calibraties>::calibrateONE(const uint32_t time, double);
template double Calibrator<ScanData, SensorGlucoseData const, Calibraties const>::calibrateONE(unsigned int, double);
#ifndef NOLOG
void showCalis(const char *name,const CaliPara *first,const uint32_t nr) {
    const int totlen=25+(24+12+2*8+10)*nr;
    char buf[totlen];
    int bufpos=0;
    bufpos=snprintf(buf,totlen,"%s: Calibrators nr=%u",name,nr);
    for(int i=0;i<nr;++i) {
        const CaliPara &cali=first[i];
        bufpos+=snprintf(buf+bufpos,totlen-bufpos,"\n%u a=%.2f b=%.2f %u",cali.time,cali.a,cali.b,cali.time);
        }
    LOGGERN(buf,bufpos);
    }
#endif


template <typename DT,typename Sens,typename Cali>
std::pair<const DT*,const DT*>      Calibrator<DT,Sens,Cali>::makecalibrated(const DT *input,DT *calibrated,int nr,bool allvalues) {
    const CaliPara *first= cali.caliPara;
    const auto caliNr=cali.caliNr;
    if(!caliNr) {
        LOGGER("makecalibrated %s: No calibrations\n",sens->shortsensorname()->data());
        if(allvalues) {
            memcpy(calibrated,input,nr*sizeof(input[0]));
            return {calibrated,calibrated+nr};
            }
        else
            return {};
        }
    
    const CaliPara *end = cali.caliPara+caliNr;
    const DT *initer=input+nr-1;
    DT *outiter=calibrated+nr-1;
    while(!initer->valid()) {
        --initer;
        --outiter;
        if(initer<input) {
            LOGGER("makecalibrated %s: no valid stream values\n",sens->shortsensorname()->data());
            return {};
            }
        }
        
    CaliPara zoek;
    zoek.time=initer->gettime();
    const CaliPara *cali=std::lower_bound(first,end,zoek,[](const CaliPara &one,const CaliPara &two) {
            return one.time<two.time;
            });
    if(cali==first) {
    #ifndef NOLOG
        time_t tim=zoek.time;
        LOGGER("makecalibrated %s %u not in interval %s\n",sens->shortsensorname()->data(),zoek.time,ctime(&tim));
#endif
        if(allvalues) {
            memcpy(calibrated,input,nr*sizeof(input[0]));
            return {calibrated,calibrated+nr};
            }
        else
            return {};
        }
    --cali;
    LOGGER("makecalibrated %s: calibrator %u for %u\n",sens->shortsensorname()->data(),cali->time,zoek.time);
    DT *endout=outiter+1; 
    for(;initer>=input;--initer) {
       if(initer->valid()) {
            while(cali->time>initer->gettime()) {
                --cali;
                if(cali<first) {
                #ifndef NOLOG
                     time_t tim=initer->gettime();;
                     LOGGER("%s before first calibration %s",sens->shortsensorname()->data(), ctime(&tim));
                #endif
                     if(allvalues) {
                        int inleft=initer-input+1;
                        DT *startpos=outiter-inleft +1;
                        memcpy( startpos ,input,inleft*sizeof(outiter[0]));
                        return {startpos,endout};
                        }
                     else
                         return {outiter+1,endout};
                    }
                }
                double calvalue=calibrateValue(*cali,*initer);
                if(isnan(calvalue)) {
                    if(allvalues) {
                       *outiter--=*initer;
                        }
                    continue;
                    }
                *outiter=*initer;
                outiter--->g=calvalue;
              }
        }
    LOGGER("end makecalibrated len=%d\n",endout-outiter-1);
     return {outiter+1,endout};
    }
template std::pair<const ScanData*,const ScanData* > Calibrator<ScanData, SensorGlucoseData const, Calibraties const>::makecalibrated(ScanData const*, ScanData*, int, bool);
template std::pair<const ScanData*,const ScanData* > Calibrator<ScanData, SensorGlucoseData, Calibraties>::makecalibrated(ScanData const*, ScanData*, int, bool);
template<typename DT,typename Sens,typename Cali>
std::pair<const DT*,const DT*>  Calibrator<DT,Sens,Cali>::makecalibratedback(const DT *input,DT *calibrated,int nr,bool allvalues) {
    const auto caliNr=cali.caliNr;
    if(!caliNr) {
        if(allvalues) {
            memcpy(calibrated,input,nr*sizeof(input[0]));
            return {calibrated,calibrated+nr};
            }
         else 
            return {};
        }
    const CaliPara *calip = cali.caliPara+caliNr-1;
    DT *outiter=calibrated;
    for(int i=0;i<nr;++i) {
        const DT &el=input[i];
        if(el.valid()) {
                double calvalue=calibrateValue(*calip,el);
                if(isnan(calvalue))
                    continue;
                *outiter=el;
                outiter++->g=calvalue;
            }
        }
    return {calibrated,outiter};
    }

template std::pair<const ScanData*,const ScanData*> Calibrator<ScanData, SensorGlucoseData, Calibraties>::makecalibratedback(ScanData const*, ScanData*, int, bool);

template std::pair<const ScanData*,const ScanData*> Calibrator<ScanData, const SensorGlucoseData, const Calibraties>::makecalibratedback(ScanData const*, ScanData*, int, bool);


template <typename DT,typename Sens,typename Cali>
int     Calibrator<DT,Sens,Cali>::caliPosAfter(const uint32_t time) {
    const uint32_t nr=cali.caliNr;
    if(!nr)  {
        LOGGER("caliPosAfter(%s,%u) no calibrators\n",sens->shortsensorname(),time);
        return 0;
        }
    const CaliPara *first = cali.caliPara;
    if(const CaliPara *calip=getCaliBefore( first,first+nr,time)) {
        int pos=calip-first+1;
        LOGGER("caliPosAfter(%s,%u)=%d\n",sens->shortsensorname(),time,pos);
        return pos;
        }
    LOGGER("caliPosAfter(%s,%u) no calibrator before time\n",sens->shortsensorname(),time);
    return 0;
    }


template double Calibrator<Glucose, SensorGlucoseData const, Calibraties const>::calibrateONE(unsigned int, double);
