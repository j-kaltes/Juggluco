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
/*      Fri Jan 27 15:25:03 CET 2023                                                 */



#ifndef LOGGER
       #include <errno.h>
       #include <stdarg.h>
       #include <string.h>
#include <stdio.h>
#define LOGGER(...)   fprintf(stderr,__VA_ARGS__)
inline void lerror(const char *str) {
        LOGGER("%s: %s\n",(char *)str,strerror(errno));
        }
inline void flerror(const char* fmt, ...) {
	constexpr const int maxbuf=80;
	char buf[maxbuf];
        va_list args;
        va_start(args, fmt);
	vsnprintf(buf,maxbuf, fmt, args);
	va_end(args);
	LOGGER("%s: %s\n",buf,strerror(errno));
	}
#endif



