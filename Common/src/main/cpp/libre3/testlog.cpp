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
char *getbytestr(uint8_t *uit,int arlen) {
  	char *strbuf=new char[arlen*3+1];
           int uitlen=0;
      for(int i=0;i<arlen;i++) {
	       uitlen+=sprintf(strbuf+uitlen," %02X",uit[i]);
	       }
	     return strbuf;
	     }
int DPGetEventLogCommand(int arg,uint8_t *command) {
        command[0]=4;
        *reinterpret_cast<int*>(command+1)=arg;
        command[5]=0;
        command[6]=0;
        return 0;
        }


int main() {
uint8_t command[7]{1,2,3,4,5,6,7};
DPGetEventLogCommand(21,command);
puts(getbytestr(command,7));

}
