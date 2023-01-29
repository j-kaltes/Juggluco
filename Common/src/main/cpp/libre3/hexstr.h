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
/*      Fri Jan 27 15:22:01 CET 2023                                                 */


#pragma once
#include <stdio.h>
class hexstr {
char *strbuf;
int len;
public:
	hexstr(const uint8_t *ar ,const int arlen): strbuf(new char[arlen*2+1]) {
                int uitlen=0;
                for(int i=0;i<arlen;i++) {
                        uitlen+=sprintf(strbuf+uitlen,"%02X",ar[i]);
                        }
		strbuf[uitlen]='\0';
		len=uitlen;
		}
	~hexstr() {
		delete[] strbuf;
		}
const char *str() const {	
	return strbuf;
	}
int size() const {
	return len;
	}
	};
