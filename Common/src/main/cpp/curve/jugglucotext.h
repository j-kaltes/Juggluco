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
#include  <utility>
#include <string_view>
#include <span>

struct Shortcut_t {const char name[12];const float value;} ;
constexpr int hourminstrlen=20;
extern char hourminstr[hourminstrlen];
typedef const char *charptr_t;
//typedef std::string_view  *string_viewar;
//typedef std::pair<const char*,const char *> errortype;
typedef std::pair<std::string_view,std::string_view> errortype;
struct jugglucotext {
char daylabel[7][12];
char monthlabel[12][15];
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
	sensorends
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
std::string_view menustr0[7], menustr1[7], menustr2[7], menustr3[7];
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
#ifndef WEAROS
std::string_view advancedstart;
bool add_s;
const std::span<const Shortcut_t> shortinit;
const std::span<const std::string_view> labels;
#endif
};

extern jugglucotext *usedtext;
