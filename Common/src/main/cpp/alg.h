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
/*      Fri Jan 27 12:35:09 CET 2023                                                 */


#ifndef ALG_H
#define ALG_H
#include <stdint.h>
#include <iostream>
    #include <sys/types.h>
       #include <sys/stat.h>
       #include <fcntl.h>
    #include <sys/mman.h>
  #include <string.h>
       #include <errno.h>
             #include <unistd.h>
//#include "history.h"
#include "logs.h"
#ifndef ANDROID__APP
#warning "ANDROID__APP not defined"
//#define  ANDROID__APP
#endif


#ifndef outf
#define outf(...) LOGGER(__VA_ARGS__)
#endif
#ifndef out
#define out(x) outf("%s\n",x)
#endif
using namespace std;
#endif
