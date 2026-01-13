/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2 and 3 sensors.                         */
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
/*      Fri Jan 27 15:20:04 CET 2023                                                 */


/*Formula to calculate GMI:
GMI(mmol/mol) = 12.71 + 4.70587 x [mean glucose in mmol/L]
GMI(%) = 3.31 + 0.02392 x [mean glucose in mg/dL]
GMI(mmol/mol) = 12.71 + 4.70587 x [mean glucose in mmol/L]
*/
#include <algorithm>
#include <numeric>
#include <math.h>
struct jugglucotext;
struct JCurve;
struct stats {
	typedef unsigned tottype;
static constexpr const	int levels[] {250,180,69,53};
	int Nr;
	int count;
	int totid;
	int counts[5]{};
	double pers[5];
	int border[2];
	double pertarget;
	double sd;
	double mean;
	double vc;
	double GMIper; 
	int GMImmol;
	double EA1Cper;
	int EA1Cmmol;
	double active;
	uint32_t starttime=UINT32_MAX,endtime=0;

template <typename GlucoseIterator> stats( std::vector<GlucoseDataType<GlucoseIterator>> &polldata) {
		auto targetlow=settings->targetlow()/10-1;
		auto targethigh=settings->targethigh()/10;
		border[0]=targethigh;
		border[1]=targetlow;
		int intarget=0;
		tottype total{};
		uint32_t minint=30;
		uint32_t prevtime=0;
		totid=0;
		for(auto [firstin,lastin,idDistance,calibrated,cali]:polldata) {
			int previd=-1;
			GlucoseIterator start=firstvalid(firstin,lastin,prevtime+minint);
			if(start==lastin)
				continue;
			if(start->gettime()<starttime) {
				starttime=start->gettime();
				}
			GlucoseIterator last=lastvalid(start,lastin-1);
			int idint=(last->getid()-start->getid())/idDistance+1;
			totid+=idint;
			if(prevtime) {
			    int32_t late= start->gettime()-(prevtime+60*idDistance);
                if(late>0) {
                    const uint32_t timeint=last->gettime()-start->gettime();
                    if(timeint>0) {
                        const int extraid=(uint64_t)late*idint/timeint;
                        totid+=extraid;
                        }
                     }
				}
			for(auto it=start;it<=last;it++) {
				if(!it->valid() ||it->getid()==previd)
					continue;
				previd=it->getid();

				int glu;
                if(double  calValue;calibrated&&!isnan(calValue=cali.forwardvalue(*it))) {
                    glu=(int)std::round(calValue);
                    }
                else {
                    glu=it->getmgdL();
                    }
				total+=glu;
				int i=0;
				for(;i<std::size(levels);i++) {
					if(glu>levels[i]) {
						break;
						}
					}
				counts[i]++;	
				if(glu>targetlow&&glu<=targethigh) {
					++intarget;
					}
				}
			prevtime=last->gettime();
			}
		if(totid==0)
			return;
		endtime=prevtime;

		count= std::reduce( begin(counts), end(counts) );
		if(count<2)
			return;
			
		active=(double)count/totid;
		std::transform(begin(counts), end(counts),pers,[this](int get){ return ((double)get)/count;});
		pertarget=((double)intarget)/count;
		prevtime=0;
		if(total==0)
			return ;
		long double mean=total/count;
		long double quadifsum=0;
		for(auto [firstin,lastin,_distance,calibrated,cali]:polldata) {
			GlucoseIterator start=firstvalid(firstin,lastin,prevtime+minint);
			if(start==lastin)
				continue;
			GlucoseIterator last=lastvalid(start,lastin-1);
			prevtime=last->gettime();
			int previd=-1;
			for(auto it=start;it<=last;it++) {
				if(!it->valid() ||it->getid()==previd)
					continue;
				previd=it->getid();
				int glu;

            if(double  calValue;calibrated&&!isnan(calValue=cali.forwardvalue(*it))) {
                glu=(int)std::round(calValue);
                }
            else {
                glu=it->getmgdL();
                }
				long double dif=(glu-mean);
				quadifsum+=dif*dif;
				}
			}
		 sd= sqrt(quadifsum/(count-1));
		 vc=sd/mean;
		GMIper = 3.31 + 0.02392 *mean; 
		GMImmol=round(12.71 + 4.70587 *mean/18.0182);
		EA1Cper= (46.7 + mean)/28.7;
		EA1Cmmol=round(( EA1Cper- 2.15)*10.929);

		this->mean=mean;
		logprint("stats::stats mean=%Lf, sd=%1f vc=%1f starttime=%u endtime=%u\n",mean,sd,vc,starttime,endtime);
		}
	void  showbar(NVGcontext* vg,JCurve &) ;
	void otherstats(NVGcontext* vg,JCurve&) ;
        void otherstats(NVGcontext* vg,JCurve &jcurve,const jugglucotext *usedtext) ;
	};
