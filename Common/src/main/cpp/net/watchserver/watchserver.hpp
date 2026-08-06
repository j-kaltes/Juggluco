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
/*      Fri Jan 27 12:38:28 CET 2023                                                 */


#pragma once
#include <string_view>

struct Getopts;

struct livecursor {
	int sensorid=-1;
	int recordid=-1;
	};

struct livestreamoptions {
	livecursor lastevent;
	bool calibrated=false;
	};

struct recdata {
	char *allbuf=nullptr;
	const char *start=nullptr;
	int len=0;
	bool livestream=false;
	livestreamoptions streamoptions;
/*	~recdata() {
                LOGGER("~recdata() %p\n",allbuf);
		delete[] allbuf;
		}*/
	const char *data() const {
		return start;
		}
	int size() const {
		return len;
		}
		
	};

using livewritefunc=bool (*)(void *context,const char *data,int len);
using livestopfunc=bool (*)(void *context);

bool streamlive(void *context,livewritefunc write,livestopfunc stopped,const livestreamoptions &options);
bool givestatistics(Getopts &opts,std::string_view origin,recdata *outdata);
