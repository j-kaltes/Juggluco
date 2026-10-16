/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2(+), Libre 3(+), Dexcom G7/ONE+,        */
/*      Sibionics GS1Sb and GS3, Accu-Chek SmartGuide, CareSens Air and              */
/*      Aidex X sensors.                                                             */
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
/*      Thu Oct 15 12:53:57 CEST 2026                                                */

// SQLite's public-domain C amalgamation is compiled privately into this module.
// Hidden symbols keep its configuration/heap limit separate from Android's
// system SQLite and other libraries. No loadable extensions or temporary files.
#define SQLITE_API __attribute__((visibility("hidden")))
#define SQLITE_OMIT_LOAD_EXTENSION 1
#define SQLITE_OMIT_SHARED_CACHE 1
#define SQLITE_THREADSAFE 1
#define SQLITE_TEMP_STORE 3
#define SQLITE_ENABLE_MATH_FUNCTIONS 1
#define SQLITE_ENABLE_PERCENTILE 1
#define SQLITE_TRUSTED_SCHEMA 0
#define SQLITE_MAX_ALLOCATION_SIZE 16777216
#define SQLITE_PRINTF_PRECISION_LIMIT 10000
#include "../vendor/sqlite/sqlite3.c"
