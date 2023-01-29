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
/*      Fri Jan 27 12:38:18 CET 2023                                                 */


#pragma once
#include <type_traits>

#include <stdint.h>
constexpr uint16_t sopen=1;
constexpr uint16_t smklen=2;
constexpr uint16_t swrite=3;
constexpr uint16_t sclose=4;
constexpr uint16_t sbackup=5;
//constexpr uint16_t srenum=6;
constexpr uint16_t sbackupstop=7;
constexpr uint16_t srender=8;
constexpr uint16_t snewnums=9;
constexpr uint16_t snums=10;
constexpr uint16_t snumnr=11;
constexpr uint16_t sack=12;
constexpr uint16_t snuminit=13;
constexpr uint16_t sasklastnum=14;
constexpr uint16_t saskfile=15;
constexpr uint16_t suptodate=16;
constexpr uint16_t sfileonce=17;
constexpr uint16_t swakeupstream=18;
constexpr uint16_t sglucose=19;
constexpr uint16_t sresetdevices=20;
constexpr uint16_t stest=99;
struct datel {
 int32_t off;
 int32_t len;
};
struct fileonce_t {
         uint16_t  com;
         uint16_t  namelen;
	 int32_t totlen;
         uint16_t  nr;
         uint16_t  dowith;
	 datel gegs[];
	};
struct renderstruct {
         uint16_t  com;
	 uint16_t type;
	 };

struct askfile {
	uint16_t com;
         uint16_t  namelen;
	 uint32_t off;
	 uint32_t len;
	char name[];
	};
struct dataonly  {
	int32_t len;
	uint8_t data[];
	};
struct asklastnum {
    const uint16_t com=sasklastnum;
    uint16_t  dbase;
	};
struct sendack {
    uint16_t com=sack;
    uint16_t  nop=1;
	};
constexpr const uint32_t ackres=917521;
typedef std::conditional<sizeof(long long) == sizeof(int64_t), long long, int64_t >::type identtype;

struct numinit {
	uint16_t com=snuminit;
	uint16_t res;
	uint32_t first;
	identtype ident;
	};

struct fileopen {
	uint16_t com;
	uint16_t rest;
	uint32_t len;
	 char name[];
	} ;//__attribute__ ((packed));
struct mklen {
	uint16_t com;
	int16_t han;
	uint32_t len;

	} ;//__attribute__ ((packed));


struct offwrite {
	uint16_t com;
	int16_t han;
	uint32_t off;
	uint32_t len;
	unsigned  char data[];
	} ;//__attribute__ ((packed));

typedef uint8_t senddata_t;

struct numsend {
	uint16_t type;
	uint8_t dbase;
	uint8_t nr;
	uint32_t totlen;
	uint32_t first; //don't set everytime
	uint32_t last; //What when sending partial data?
	uint32_t nums[];
	};
struct lastpos_t {
        uint16_t type;
        uint16_t dbase;
        uint32_t lastpos;
        };


#include "misc.h"

