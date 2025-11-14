/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2, Libre 3, Dexcom G7/ONE+ and           */
/*      Sibionics GS1Sb sensors.                                                     */
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
/*      Fri Jun 20 17:38:39 CEST 2025                                                 */


#include <string>
#include <stdio.h>
#include <algorithm>
#include "qr.h"
#include "logs.hpp"
extern std::string mkbackjson(int pos) ;

static const char *getchar(bool upper,bool lower) {
    int index=upper<<1|lower;
    switch(index) {
        case 1: return "\u2584";
        case 2: return "\u2580";
        case 3: return "\u2588";
        }
    return " ";
    }

void print_qr(FILE *fp,const qr::Qr &codec) {
    printf("\n\n\n\n");
    const int len= codec.side_size();
    int y;
    if(len%2) {
        fprintf(fp,"        ");
        for (int x = 0;x < len; ++x) {
            const char *ch=getchar(false,codec.module(x, 0));
            fputs(ch,fp);
            }
        fprintf(fp,"\n");
        y=1;
        }
   else {
        y=0;
        }
    for(; y < len; y+=2) {
        fprintf(fp,"        ");
        for (int x = 0;x < len; ++x) {
            const char *ch=getchar(codec.module(x, y),codec.module(x, y+1) );
            fputs(ch,fp);
            }
        fprintf(fp,"\n");
        }
    fprintf(fp,"\n\n\n\n");
}

/*
void print_qr(FILE *fp,const qr::Qr &codec) {
    printf("\n\n\n\n");
    const int len= codec.side_size();
    for (int y = 0; y < len; y+=2) {
        fprintf(fp,"        ");
        for (int x = 0; x < codec.side_size(); ++x) {
            const int nexty=y+1;
            bool lower= (nexty>=len)?false:codec.module(x, nexty);
            const char *ch=getchar(codec.module(x, y),lower );
            fputs(ch,fp);
            }
        fprintf(fp,"\n");
    }
    fprintf(fp,"\n\n\n\n");
}
*/
static constexpr int version(int by) {
    static constexpr const int bytes[]={33, 54, 79, 107, 135, 155, 193, 231, 272, 322, 368, 426, 459, 521, 587, 645, 719, 793, 859, 930, 1004, 1092, 1172, 1274, 1368, 1466, 1529, 1629, 1733, 1841, 1953, 2069, 2189, 2304, 2432, 2564, 2700, 2810};
    constexpr auto en=std::end(bytes);
    const auto hit=std::lower_bound(bytes,en,by);
    if(hit==en)
        return 40;
    return hit-bytes+3;
    }


static bool printQR(FILE *fp,std::string_view str) {
    for(int ver=version(str.size());ver<=40;++ver) {
            qr::Qr qr(ver);
            if(qr.encode(str.data(),str.size(),qr::Ecc::L, -1)) {
                print_qr(fp,qr);
                return true;
                }
             else
                LOGGER("Encoding failed %s len=%ld version=%d\n",str.data(),str.size(),ver);
          }
    return false;
}


extern bool mkAutodumpQR();
extern bool dumpQR(int pos);
bool dumpQR(int pos) {
     std::string json=mkbackjson(pos);
     if(json.size()<10) {
        fprintf(stderr,"Making JSON for pos=%d failed\n",pos);
        return -1;
        }
     return printQR(stdout,json);
    }


extern int makeHomeBackupReceiver() ;
extern int makeHomeBackupSender() ;
extern bool mkAutodumpQRReceiver() ;
extern bool mkAutodumpQRSender() ;
extern bool mkAutoICEQRReceiver() ;
extern bool mkAutoICEQRSender() ;
extern int makeICEreceiver() ;
extern int makeICEsender() ;
bool mkAutodumpQRReceiver() {
    const int pos=makeHomeBackupReceiver(); 
      if(pos<0) {
        fprintf(stderr,"Can't auto generate Receiver connection\n");
        return false;
        }
    return  dumpQR(pos);
    }
bool mkAutodumpQRSender() {
    const int pos=makeHomeBackupSender(); 
      if(pos<0) {
        fprintf(stderr,"Can't auto generate Sender connection\n");
        return false;
        }
    return  dumpQR(pos);
    }
bool mkAutoICEQRReceiver() {
    const int pos=makeICEreceiver(); 
      if(pos<0) {
        fprintf(stderr,"Can't auto generate Receiver connection\n");
        return false;
        }
    return  dumpQR(pos);
    }
bool mkAutoICEQRSender() {
    const int pos=makeICEsender(); 
      if(pos<0) {
        fprintf(stderr,"Can't auto generate Sender connection\n");
        return false;
        }
    return  dumpQR(pos);
    }
