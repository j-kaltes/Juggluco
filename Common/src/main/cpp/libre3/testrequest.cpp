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


#include <stdio.h>
#include <stdint.h>
struct RequestHistory {
	int8_t kind[2];
	int8_t arg;
	int32_t from;
	} __attribute__ ((packed));
//com.adc.trident.app.frameworks.mobileservices.libre3.libre3DPCRLInterface::DPPatchControlHistoricalDataBackfillGreaterEqual
int historyControl(int arg,int from,uint8_t *command) {
	*reinterpret_cast<RequestHistory*>(command)={.kind={1,0},.arg=static_cast<int8_t>(arg),.from=from};
	return 0;
	}


char *getbytestr(uint8_t *uit,int arlen) {
  	char *strbuf=new char[arlen*3+1];
           int uitlen=0;
      for(int i=0;i<arlen;i++) {
	       uitlen+=sprintf(strbuf+uitlen," %02X",uit[i]);
	       }
	     return strbuf;
	     }
int main() {
	constexpr const	int arlen= sizeof(RequestHistory);
	uint8_t uit[arlen];
	historyControl(1,19640,uit);
	char *strbuf=getbytestr(uit,arlen);
	puts(strbuf);
	delete[] strbuf;
	
	}
