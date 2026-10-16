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


#include "nums/numdata.hpp"
#include <vector>
std::vector<Numdata*> numdatas;

#include "net/passhost.hpp"
#include "net/makerandom.hpp"
#include "net/Connect.hpp"
#include "activity/mirror.hpp"
namespace ja=juggluco::activity;

// Bind exact mirrored signatures to this clone's databases. Copied numeric
// indices alone never own an activity, and ordinary Amounts are not scanned.
void bindMirroredActivityAmounts(ja::Store &store) {
   if(!store.header().receivedMirror)return;
   struct Database {Numdata *data;uint32_t base;int64_t identity;};
   std::vector<Database> databases;
   for(auto *data:numdatas)databases.push_back({data,uint32_t(data->getindex()),data->getnewident()});
   for(uint32_t i=1;i<=store.header().count;++i) {
      const auto *owner=store.get(i);
      const auto saved=store.view<ja::AmountBinding>(owner->amountBinding);
      if(saved.size()!=1||saved[0].number.mealptr!=(Num::sportbit|i))continue;
      const auto binding=saved[0];bool found=false;
      for(bool preferred:{true,false}) {
         for(const auto &database:databases)if((database.base==binding.base)==preferred) {
            std::lock_guard lock(database.data->nummutex);
            for(auto *number=database.data->firstnotless(binding.number.time);
                  number<database.data->end()&&number->time==binding.number.time;++number) {
               if(database.data->valid(number)&&number->type!=settings->data()->bloodvar&&
                     !memcmp(number,&binding.number,sizeof(Num))) {
                  // Database identity is local bookkeeping, not a new activity
                  // revision. Rebasing in place avoids ever-growing bindings and
                  // metadata echo loops between mirrors that send both ways.
                  auto *local=const_cast<ja::AmountBinding*>(saved.data());
                  local->base=database.base;local->database=database.identity;
                  found=true;break;
               }
            }
            if(found)break;
         }
         if(found)break;
      }
   }
}
bool receiveActivityMirror(const uint8_t *data,size_t length) {
   try {
      auto &store=ja::globalStore();std::lock_guard lock(store.mutex);
      ja::mirror::receive(store,{reinterpret_cast<const std::byte*>(data),length});
      bindMirroredActivityAmounts(store);
      setnumchanged();if(backup)backup->wakebackup(wakenums);
      LOGGER("Activity: received mirrored records=%u\n",store.header().count);
      return true;
   } catch(const std::exception &error) {LOGGER("Activity mirror receive: %s\n",error.what());return false;}
}
int updateactivities(crypt_t *pass,Connect *connect,changednums &numbers) {
   const auto cursor=numbers.activityState;
   try {
      ja::mirror::Transfer transfer;
      {
         auto &store=ja::globalStore();std::lock_guard lock(store.mutex);
         if(ja::mirror::unchanged(store,cursor))return 2;
         transfer=ja::mirror::capture(store,cursor);
      } // Release the activity mutex before waiting on the network.
      const std::vector<subdata> data={{reinterpret_cast<const uint8_t*>(transfer.bytes.data()),0,int(transfer.bytes.size())}};
      if(!connect->senddata(pass,data,ja::mirror::filename)) {
         numbers.activityState={};return 0;
      }
      numbers.activityState=transfer.cursor;
      LOGGER("Activity: mirrored %zu bytes\n",transfer.bytes.size());return 1;
   } catch(const std::exception &error) {
      LOGGER("Activity mirror send: %s\n",error.what());return 0;
   }
}
extern void makenightswitch();
void prunenums() {
    for(auto *num:numdatas) {
        num->prunenums();
        }
    }
void makenightswitch() {
  LOGAR("makenightswitch");
       if(settings->data()->initVersion<28) {
          if(settings->data()->initVersion<26) {
             if(settings->data()->initVersion<25) {
                if(settings->data()->initVersion<24) {
                   if(settings->data()->initVersion<23) {
          //            LOGAR("makenightswitch");
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
    //     settings->data()->initVersion=28;
            }

     //prunenums();
#if !defined(NOLOG) && defined( __ANDROID_API__)
extern void checker();
   checker();
#endif
   }
//bool update(int sock,int &len, struct numspan *ch) 
int updatenums(crypt_t*pass,Connect *connect,struct changednums *nums,int ind) {
   int ret=0;
   for(int i=0;i<numdatas.size();i++) {
      if(int subret=numdatas[i]->update(pass,connect,nums,ind)) 
         ret|=subret;
      else
         return 0;
      }
   const int mealsSent=meals->datameal()->updatemeal(pass,connect,nums[1].lastmeal);
   if(!mealsSent)return 0;
   const int activitiesSent=updateactivities(pass,connect,nums[1]);
   return activitiesSent?ret|mealsSent|activitiesSent:0;
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
  const int base=data->dbase;
   numdatas[base]-> receivelastpos(base,data->lastpos);
   return true;
   }

bool backupnums(const struct numsend* innums) { 
   if(numdatas.size()<2)
      return false;
   if(!numdatas[innums->dbase]->backupnums(innums))return false;
   // Activity metadata can arrive before or after a changed Amount packet.
   if(access((std::string(globalbasedir)+"/activities.dat").c_str(),F_OK)==0) {
      try {auto &store=ja::globalStore();std::lock_guard lock(store.mutex);bindMirroredActivityAmounts(store);}
      catch(const std::exception &error){LOGGER("Activity mirror bindings: %s\n",error.what());}
   }
   return true;
   }
bool backupnuminit(const numinit *numst) {
   if(numdatas.size()<2)
      return false;
   return numdatas[(bool)(numst->ident)]->numbackupinit(numst);
   }

bool numsbackupsendinit(crypt_t*pass,Connect *connect,struct changednums *nuall,uint32_t starttime) {
   LOGGER("numsbackupsendinit sock=%d starttime=%u  numdatas.size()=%d\n",connect->getSenderIdent(),starttime,numdatas.size());
   for(auto*el:numdatas) 
      if(!el->backupsendinit(pass,connect,nuall,starttime) )
         return false;
   nuall[1].activityState={};
   return true;
   }
int sendlastnum(const int dbase) {
   if(dbase>=numdatas.size())
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
