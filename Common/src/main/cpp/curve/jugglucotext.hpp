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


//	s/^.*showerror(vg, *\("[^"]*"\), *\("[^"]*"\).*/{\1,\2},/g
#pragma once
#include "config.h"
#include  <utility>
#include <string_view>
#include <span>
#include <stdint.h>
#include <unordered_map>

#define INJUGGLUCO JUGGLUCO_APP

using namespace std::literals;

#ifdef INJUGGLUCO
struct Shortcut_t {const char name[12];const float value;} ;
constexpr int hourminstrlen=25;
extern char hourminstr[hourminstrlen];
typedef const char *charptr_t;
typedef std::pair<std::string_view,std::string_view> errortype;
#endif
struct jugglucotext {
char daylabel[7][12];
#ifdef INJUGGLUCO
const char speakdaylabel[7][24];
#endif
char monthlabel[12][15];
#ifdef INJUGGLUCO
std::string_view scanned;
std::string_view readysecEnable;
std::string_view readysec;
std::string_view networkproblem;
std::string_view enablebluetooth;
std::string_view useBluetoothOff;
std::string_view noconnectionerror;
std::string_view stsensorerror;
std::string_view streplacesensor;
std::string_view endedformat;
std::string_view notreadyformat;

#ifndef WEAROS
std::string_view median,middle;
#endif
std::string_view history,historyinfo, history3info,
	sensorstarted,
	lastscanned,
	laststream,
	sensorends,
	sensorexpectedend
#ifndef WEAROS
,
	newamount,
	averageglucose;
charptr_t
	duration,
	timeactive,
	nrmeasurement,
	EstimatedA1C,
	GMI,
	SD,
	glucose_variability;
std::string_view menustr0[8], menustr1[7], menustr2[7], menustr3[7];
std::string_view *menustr[4]={menustr0,menustr1,menustr2,menustr3};
#else
;
std::string_view	amount;
std::string_view menustr0[5], menustr2[4];
std::string_view *menustr[2]={menustr0,menustr2};
#endif
errortype scanerrors[0x11];

errortype libre3scanerror;
errortype libre3wrongID;
errortype libre3scansuccess;
errortype unknownNFC;
errortype nolibre3;
errortype libre3zeroID;
std::string_view needsandroid8;
#ifndef WEAROS
std::string_view advancedstart;
bool add_s;
const std::span<const Shortcut_t> shortinit;
const std::span<const std::string_view> labels;
#endif
#ifndef DONTTALK
charptr_t  checked,unchecked;
std::string_view Undetermined, FallingQuickly,Falling, Stable, Rising, RisingQuickly;
#endif
//#ifndef WEAROS
//std::string_view sibionics;
//#endif
std::string_view receivingpastvalues;
std::string_view receivingdata;
std::string_view unsupportedSibionics;
std::string_view waitingforconnection;
std::string_view deleted;
std::string_view nolocationpermission;
std::string_view nonearbydevicespermission;
#else
std::string_view statistics;
#endif
std::string_view summarygraph;
std::string_view logdays;
std::string_view unhide;
#ifdef INJUGGLUCO
#ifndef DONTTALK
std::string_view getTrendName(int type) const {
   if(type>=0&&type<6) {
        const std::string_view *trends=&Undetermined; 
        return trends[type];
        }
    else {
        return "ERROR";
        }
   }
#endif
#endif
#ifndef WEAROS
std::string_view statisticsName() const {
    return 
    #ifdef INJUGGLUCO
    menustr1[4];
    #else
    statistics;
    #endif
    }
#endif
};

extern const jugglucotext *usedtext;
extern const jugglucotext engtext;
#include <ctype.h>
#define mklanguagenum2(a,b) a|b<<8
#define mklanguagenum(lang) mklanguagenum2(lang[0],lang[1])
#define mklanguagenumlow(lang) mklanguagenum2(tolower(lang[0]),tolower(lang[1]))
class language {
    inline static std::unordered_map<uint16_t,const jugglucotext*> langmap;
    public:
    language(const char *name,const jugglucotext *data) {
         langmap.insert({mklanguagenum(name),data});
         }
    static const jugglucotext* gettext(uint16_t lang)  {
    if(auto hit = langmap.find(lang); hit != langmap.end())
            return hit->second;
        return &engtext;
        }
    static const jugglucotext* gettext(const char *name)  {
        return gettext(mklanguagenumlow(name));
        }
    };
#define addlang(code) language language_##code(#code,&code##text);
