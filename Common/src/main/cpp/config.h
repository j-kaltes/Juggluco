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


#ifndef CONFIG_H
#define CONFIG_H

//#define DATALIB 1


#define HASP1 1
#define CARRY_LIBS 1
#define NEWUS 1

#endif
#ifndef APPSUFFIX
#define APPSUFFIX ""
#endif
#ifdef APPID 
#define BASEDIR "/data/data/" APPID APPSUFFIX "/files"
#else
#ifndef BASEDIR
	#if RELEASEDIR==1
	#define BASEDIR "/data/data/tk.glucodata/files"
	#else
	#ifdef INSDCARD
	#define BASEDIR "/sdcard/glucodata/"
	#else
	#define BASEDIR "/data/data/tk.glucodata.debug/files"
	#endif
	#endif
#endif

#define FILESDIR BASEDIR
//#define CHANGECODE=1
#define REALPATH 1

#endif

#if defined(NEWUS) 
#ifdef ALWAYSPATH 
#define NEEDSPATH 1
#endif
#endif
#if defined(LIBRE3)
#define NEEDSPATH 1
#endif
