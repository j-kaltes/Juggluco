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
/*      Fri Jan 27 12:37:28 CET 2023                                                 */


#ifndef GROUPTYPE_H
#define GROUPTYPE_H
#if __cplusplus < 201703L
#define cpluspluschange
#pragma push_macro("__cplusplus")
#undef __cplusplus  
#define __cplusplus  201703L
#endif // _cplusplus 

#include <string_view>

#ifdef cpluspluschange
#pragma pop_macro("__cplusplus")
#undef cpluspluschange
#endif

#define Pragmessage(x) _Pragma (#x)
#define  message(x)  Pragmessage(message #x)

#ifdef ARRAY_VIEW1D
using Grouptype= array_view1d<const uint32_t>;
#warning use array_view1d
#else
#ifdef EXPERIMENTAL
message("std::experimental::basic_string_view")  

using Grouptype= std::experimental::basic_string_view<const uint32_t>;
#else
//#warning use  basic_string_view
using Grouptype= std::basic_string_view<const uint32_t>;
#endif // EXPERIMENTAL
#endif //ARRAY_VIEW1D
#endif // GROUPTYPE_H
