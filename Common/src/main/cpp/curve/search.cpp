#include "config.h"
#include "JCurve.hpp"

#include "numdisplay.hpp"
#include "searchgegs.hpp"
#include "SensorGlucoseData.hpp"
#include "calibrate/Calibrate.hpp"
extern vector<NumDisplay*> numdatas;
extern Searchgegs searchdata;
Searchgegs searchdata={.type=nosearchtype};
extern int carbotype;
extern pair<const ScanData*,const ScanData*> getScanRange(const ScanData *scan,const int len,const uint32_t start,const uint32_t end);
void JCurve::highlightnum(const Num *num) {
    uint32_t tim=num->time;
#ifndef WEAROS
    if(numlist)
        numpagenum(tim) ;
    else
#endif
        setstarttime(starttimefromtime(tim));
    }


const Num * NumDisplay::findpast(const Num *high) const {
	const Num *low=begin();
	for(const Num *it=high-1;it>=low;it--) 
		if(searchdata(it))
			return it;
	return nullptr;
	}
template <int N>
const Num * NumDisplay::findpast(JCurve &j) const {
	return findpast(std::get<N>(j.extrums[numdatasPos]));
	}
template <int N>
const Num * NumDisplay::findforward(JCurve &j) const {
	const Num *en=end();
	const Num *start=std::get<N>(j.extrums[numdatasPos]);
	if(const Num *hit=find_if(start,en,searchdata);hit<en) {
		return hit;
		}
	return nullptr;
	}



template <int N>
const Num *JCurve::findpast() {
    const Num  *hit=nullptr;
    const int len=numdatas.size();
    int i=0;
    for(;i<len;i++) {
        if((hit=numdatas[i]->findpast<N>(*this)))
            break;
        }
    if(hit) {
        for(;i<len;i++) {
            if(const Num *mog=numdatas[i]->findpast<N>(*this);mog&&mog->time>hit->time)
                hit=mog;
            }
        }
    return hit;
    }

template <int N> const Num *JCurve::findforward()  {
    const Num  *hit=nullptr;
    const int len=numdatas.size();
    int i=0;
    for(;i<len;i++) 
        if((hit=numdatas[i]->findforward<N>(*this)  ))
            break;

    if(hit) {
        for(;i<len;i++)  {
            if(const Num *mog=numdatas[i]->findforward<N>(*this);mog&&mog->time<hit->time)
                hit=mog;
            }
        }
    return hit;
    }


int JCurve::nextpast() {
    if(searchdata.type&glucosetype) {
         return glucosesearch(0,starttime-1);
        }
    else {
        if(const Num *hit=findpast<0>()) {
            highlightnum(hit);
            return 0;
            }
        }
    return 1;
    }


int JCurve::nextforward() {
    if(searchdata.type&glucosetype) {
        return glucoseforwardsearch(starttime+duration, std::numeric_limits<uint32_t>::max());
        }
    else {
        if(const Num *hit=findforward()) {
            highlightnum(hit);
            return 0;
            }
        }
    return 1;            
    }
void stopsearch() {
    searchdata.type=nosearchtype;
    }
static const ScanData * findScan(const ScanData *start,const ScanData *en) {
    for(const ScanData *it=en-1;it>=start;--it) {
        if(searchdata(it))
            return it;
        }
    return nullptr;
 }

static const ScanData * findCalibratedScan(const SensorGlucoseData  *sens,const ScanData *start,const ScanData *en) {
  CalibrateBackward  cali(sens,0); 
  if(!cali.size())
      return nullptr;
  for(const ScanData *it=en-1;it>=start;--it) {
        double  calValue=cali.backvalue(*it);
        if(isnan(calValue))
            return nullptr;
        if(searchdata(it,calValue))
            return it;
        }
   return nullptr;
 }
static const Glucose * findhistory(const SensorGlucoseData  * hist, const uint32_t firstpos, const uint32_t lastpos) {
    for(auto pos=lastpos;pos>=firstpos;--pos)  {
        const Glucose *g=hist->getglucose(pos);
        if(searchdata(g))
            return g;

        }
    return nullptr;
    }
void JCurve::glucosesel(uint32_t tim) {
    if(tim>starttime&&tim<timeend())
        return;
    setstarttime(starttimefromtime(tim));
    }


#ifndef NDEBUG
void logglucose(const char *str,const Glucose *glu) {
    const time_t tim=glu->gettime();
    LOGGER("%s: %d %.1f %s",str, glu->id,glu->getsputnik()/100.0f,ctime(&tim));
    }
#else
#define    logglucose(x,y)
#endif
uint32_t JCurve::glucosesearch(uint32_t starttime,uint32_t endtime) {
    LOGGER("glucosesearch(%u,%u)\n",starttime, endtime);
    uint32_t hittime=starttime;
    const Glucose *histhit=nullptr;
    const ScanData *scanhit=nullptr;
    for(int it=sensors->last();it>=0;it--) {
        const sensor *sen=sensors->getsensor(it);
        LOGGER("Sensor %s\n",sen->name);
        if(sen->starttime>endtime)
            continue;
        if(sen->endtime&&sen->endtime<starttime)
            break;
        if(auto his=sensors->getSensorData(it)) {
            int32_t lastpos= his->getAllendhistory()-1;
            uint32_t tim=0;
            int32_t firstpos = his->getstarthistory();
            if(his->isDexcom()||his->isSibionics()) {
                goto skiphistory;
                 }
            if(lastpos<firstpos) {
                goto skiphistory;
                }
            for(;!(tim=his->getglucose(lastpos)->gettime());lastpos--) {
                if(lastpos<=firstpos)
                    break;
                }
            if(tim<hittime)  {
                //continue;
                goto skiphistory;
                }
            if((searchdata.type&glucosetype)==glucosetype) {
                if((searchdata.type&historysearchtype)==historysearchtype) {
                    int endpos;
                    if(tim<endtime)
                        endpos=lastpos;
                    else {
                        int period=his->getinterval();
                        endpos=lastpos-(tim-endtime)/period;
                        if(endpos<1)
                            endpos=1;    
                        }
                    while(endpos<lastpos&&    his->timeatpos(endpos)<endtime)
                        endpos++;
                    uint32_t tmptim;
                    while(!(tmptim=his->timeatpos(endpos))||tmptim>=endtime) {
                        endpos--;
                        if(endpos<=firstpos)
                            goto skiphistory;
                        }

                    int startpos=his->gettimepos(hittime);
                    if(startpos<1) 
                        startpos=1;
                    else {
                        
                        while(startpos>1&&(!(tmptim=his->timeatpos(startpos))||tmptim>=hittime))
                            startpos--;
                        while(startpos<endpos&&    his->timeatpos(startpos)<hittime)
                            startpos++;
                        }
                     const Glucose *mog=findhistory(his,startpos,endpos); 
                     if(mog&&mog->gettime()>hittime) {
                        histhit=mog;
                        hittime=mog->gettime();
                        logglucose("glucosesearch mog ",mog);
                        }
                    }
                skiphistory:
                if((searchdata.type&scansearchtype)==scansearchtype) {
                    std::span<const ScanData>     scan=his->getScandata();
                    auto [under,above] =getScanRange(scan.data(),scan.size(),hittime,endtime) ;
                    const ScanData *mogscan=findScan(under,above);
                    if(mogscan&&mogscan->t>hittime) {
                        scanhit=mogscan;
                        hittime=mogscan->t;
                        }
                        }
                if(searchdata.type&12) {
                    std::span<const ScanData>     scan=his->getPolldata();
                    auto [under,above] =getScanRange(scan.data(),scan.size(),hittime,endtime) ;
                    if((searchdata.type&streamsearchtype)==streamsearchtype) {
                        const ScanData *mogscan=findScan(under,above);
                        if(mogscan&&mogscan->t>hittime) {
                            scanhit=mogscan;
                            hittime=mogscan->t;
                            }
                           }
                    if((searchdata.type&calibratedStreamsearchtype)==calibratedStreamsearchtype) {
                        const ScanData *mogscan=findCalibratedScan(his,under,above);
                        if(mogscan&&mogscan->t>hittime) {
                            scanhit=mogscan;
                            hittime=mogscan->t;
                            }
                           }
                }
              }
           }
       }

    uint32_t res;
    if(!histhit)    {
        if(!scanhit)  {
            LOGSTRING("no hist and no scanhit\n");
            return 1;
            }
        LOGSTRING("no scan hist and but scanhit\n");
        res=scanhit->t;
        }
    else {
        LOGSTRING("hist hit\n");
        if(!scanhit||histhit->time>scanhit->t)
            res=histhit->time;
        else
             res=scanhit->t;
         }
    LOGGER("glucosesearch found %d\n",res);
    glucosesel(res);
    return 0;
    }

static const ScanData * findforwardCalibratedScan(const SensorGlucoseData  *sens, const ScanData *start,const ScanData *en) {
  CalibrateForward  cali(sens,0); 
  if(!cali.size())
     return nullptr;
  for(const ScanData *it=start;it<en;++it) {
        double  calValue=cali.value(*it);
        if(!isnan(calValue)&&searchdata(it,calValue))
            if(searchdata(it,calValue))
                return it;
        }
   return nullptr;
 }

static const ScanData * findforwardScan(const ScanData *start,const ScanData *en) {
    for(const ScanData *it=start;it<en;++it) {
        if(searchdata(it))
            return it;
        }
    return nullptr;
 }
static const Glucose * findforwardhistory(const SensorGlucoseData  * hist, const uint32_t firstpos, const uint32_t lastpos) {
    for(auto pos=firstpos;pos<=lastpos;++pos)  {
        const Glucose *g=hist->getglucose(pos);
        if(searchdata(g))
            return g;

        }
    return nullptr;
    }

uint32_t JCurve::glucoseforwardsearch(uint32_t starttime,uint32_t endtime) {
    uint32_t hittime=endtime;
    const Glucose *histhit=nullptr;
    const ScanData *scanhit=nullptr;
    for(int it=0;it<=sensors->last();it++) {
        const sensor *sen=sensors->getsensor(it);
        LOGGER("Sensor %s\n",sen->name);
        if(sen->starttime>hittime)
            break;
        if(sen->endtime<starttime)
            continue;
        auto his=sensors->getSensorData(it);
        int32_t lastpos= his->getAllendhistory()-1;
        int32_t firstpos= his->getstarthistory();
        uint32_t tim=0;
        if(his->isDexcom()||his->isSibionics()) {
                goto skiphistory;
                 }
        if(lastpos<firstpos) {
            goto skiphistory;
            
            }
        for(;!(tim=his->getglucose(lastpos)->gettime());lastpos--) {
            if(lastpos<=firstpos)
                break;
            }
                
        if(tim<starttime) {
            goto skiphistory;
            }

    if((searchdata.type&historysearchtype)==historysearchtype) {
        int endpos;
        if(tim<hittime)
            endpos=lastpos;
        else {
            int period=his->getinterval();
            endpos=lastpos-(tim-hittime)/period;
            if(endpos<1)
                endpos=1;    
            }
        while(endpos<lastpos&&    his->timeatpos(endpos)<hittime)
            endpos++;
        uint32_t tmptim;
        while(!(tmptim=his->timeatpos(endpos))||tmptim>=endtime) {
            endpos--;
            if(endpos<=firstpos)
                goto skiphistory;
            }

        int startpos=his->gettimepos(starttime);
        if(startpos<1) 
            startpos=1;
        else {
              uint32_t tmptim;
            while(startpos>1&&(!(tmptim=his->timeatpos(startpos))||tmptim>=starttime))
                startpos--;
            while(startpos<endpos&&    his->timeatpos(startpos)<starttime)
                startpos++;
            }
         const Glucose *mog=findforwardhistory(his,startpos,endpos); 
         if(mog&&mog->gettime()<hittime) {
            histhit=mog;
            hittime=mog->gettime();
            }
        }
    skiphistory:
    if((searchdata.type&scansearchtype)==scansearchtype) {
        const std::span<const ScanData>     scan=his->getScandata();
        auto [under,above] =getScanRange(scan.data(),scan.size(),starttime,hittime) ;
        const ScanData *mogscan=findforwardScan(under,above);
        if(mogscan&&mogscan->t<hittime) {
            scanhit=mogscan;
            hittime=mogscan->t;
            }
               }
     if(searchdata.type&12) {
        const std::span<const ScanData>     scan=his->getPolldata();
        auto [under,above] =getScanRange(scan.data(),scan.size(),starttime,hittime) ;
        if((searchdata.type&streamsearchtype)==streamsearchtype) {
            const ScanData *mogscan=findforwardScan(under,above);
            if(mogscan&&mogscan->t<hittime) {
                scanhit=mogscan;
                hittime=mogscan->t;
                }
              }
        if((searchdata.type&calibratedStreamsearchtype)==calibratedStreamsearchtype) {
            const ScanData *mogscan=findforwardCalibratedScan(his,under,above);
            if(mogscan&&mogscan->t<hittime) {
                scanhit=mogscan;
                hittime=mogscan->t;
                }
              }
            }




       }



    uint32_t res;
    if(!histhit)    {
        if(!scanhit) 
            return 1;
        res=scanhit->t;
        }
    else {
        if(!scanhit||histhit->time<scanhit->t)
            res=histhit->time;
        else
             res=scanhit->t;
         }

    glucosesel(res);
    return 0;
    }
int JCurve::searchcommando(int type, float under,float above,int frommin,int tomin,bool forward,const char *regingr,float amount) {
if(type&glucosetype) {
    searchdata={type ,backconvert(under), backconvert(above), frommin, tomin,0};
    return forward?glucoseforwardsearch(starttime, std::numeric_limits<uint32_t>::max()):glucosesearch(0,starttime+duration);
    }
uint32_t  maxlab= settings->getlabelcount();
if(type>=maxlab)
    type=0x80000000;
searchdata={ type, under, above, frommin, tomin,maxlab};
if(regingr!=nullptr&&type==carbotype) {
    meals->datameal()->searchingredients(regingr,searchdata.ingredients);
    if(searchdata.ingredients.size()==0) {
        searchdata.type=nosearchtype;
        return 4;
        }
    searchdata.amount=amount;
    }
else
    searchdata.ingredients.clear();
if(const Num *hit=forward?findforward<0>():findpast<1>()) {
    highlightnum(hit);
    return 0;
    }
searchdata.type=nosearchtype;
return 1;
}


template <typename T>
bool searchhit(const T *ptr) {
    return searchdata(ptr);
    }

template   bool searchhit<Num>(const Num *ptr); 
