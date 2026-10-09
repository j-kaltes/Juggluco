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
/*      Fri Jan 27 12:36:44 CET 2023                                                 */


#include <stdint.h>
#pragma once
typedef float float32_t;
struct Num {
  uint32_t time;
  union {
      uint32_t mealptr;
      uint32_t sportindex; // Opaque tagged reference; round-trips through Kerfstok.
      uint32_t librenr;
      struct {
        uint32_t calibration:31;
        bool exclude:1;
        };
      };
  float32_t value;
  uint32_t type;
 static constexpr uint32_t sportbit=0x80000000u;
 static constexpr bool issportref(uint32_t ref) { return (ref&sportbit)!=0; }
 const uint32_t gettime() const {
     return time;
    };
  bool calibrator(int t) const {
        return t==type&&!exclude;
        }
  bool nearprevious() const {
        const Num *prev=this-1;
        return prev->type==type&&prev->value==value&&((time-prev->time)<3*60);
        }
  };
