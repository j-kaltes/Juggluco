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
/*      Fri Jan 27 12:36:44 CET 2023                                                 */


#include "nums/numdata.h"
#include <vector>
std::vector<Numdata*> numdatas;

#include "net/passhost.h"
#include "net/makerandom.h"
extern void makenightswitch();

void makenightswitch() {
   if(settings->data()->initVersion<28) {
      if(settings->data()->initVersion<26) {
         if(settings->data()->initVersion<25) {
            if(settings->data()->initVersion<24) {
               if(settings->data()->initVersion<23) {
                  LOGAR("makenightswitch");
                  settings->data()->postTreatments=false;
                  for(auto *num:numdatas) {
                     num->getnightSwitch() =num->getlastpos();
                     }
                  }
               makerandom(&settings->data()->jugglucoID,sizeof(settings->data()->jugglucoID));
               for(auto *num:numdatas) {
                  num->getnightIDstart() =num->getlastpos();
                  }

               }
              }
//           settings->data()->threshold=0.8f;
            }
       extern         void resensordata(int sensorindex);
        resensordata(0);
     settings->data()->initVersion=28;
        }
	}

//bool update(int sock,int &len, struct numspan *ch) 
int updatenums(crypt_t*pass,int sock,struct changednums *nums,int ind) {
	int ret=0;
	for(int i=0;i<numdatas.size();i++) {
		if(int subret=numdatas[i]->update(pass,sock,nums,ind)) 
			ret|=subret;
		else
			return 0;
		}
#ifdef USE_MEAL
	if(int did=meals->datameal()->updatemeal(pass,sock,nums[0].lastmeal))
		return ret|did;
	return 0;
#else	
	return ret;
#endif
	}
	/*
void remakenums() {
	for(auto*el:numdatas) 
		el->remake();
	} */

bool happened(uint32_t stime,int type,float value)  {
	for(auto*el:numdatas) 
		if(el->happened(stime,type,value))
			return true;
	return false;
	}


bool receivelastpos(const lastpos_t *data) {
	if(numdatas.size()<2)
		return false;
	numdatas[data->dbase]-> receivelastpos(data->lastpos);
	return true;
	}

bool backupnums(const struct numsend* innums) { 
	if(numdatas.size()<2)
		return false;
	return numdatas[innums->dbase]->backupnums(innums);
	}
bool backupnuminit(const numinit *numst) {
	if(numdatas.size()<2)
		return false;
	return numdatas[(bool)(numst->ident)]->numbackupinit(numst);
	}

bool numsbackupsendinit(crypt_t*pass,int sock,struct changednums *nuall,uint32_t starttime) {
	for(auto*el:numdatas) 
		if(!el->backupsendinit(pass,sock,nuall,starttime) )
			return false;
	return true;
	}
int sendlastnum(const int dbase) {
	if(numdatas.size()<2)
		return -1;
	return numdatas[dbase]->getlastpos();
	}

uint32_t getnumlasttime() {
	uint32_t last=0u;
	for(auto el:numdatas)  {
		auto mog=el->getlasttime();
		if(mog>last)
			last=mog;	
		}
	return last;
	}

#ifdef LIBRENUMBERS
extern pathconcat numbasedir;
Mmap<Num> librenumsdeleted;
void initlibreview() {
        const auto elsize=sizeof(Num);
        librenumsdeleted=Mmap<Num>(numbasedir,"librenumsdeleted",4096*elsize);
        }
void setlibrenum3(bool libre3) {
	if(libre3==settings->data()->libre3nums)
		return;
	settings->data()->libre3nums=libre3;
	for(auto*el:numdatas) 
		el->changeDevice();
	}
#endif
