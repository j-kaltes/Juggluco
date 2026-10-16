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
/*      Thu Oct 15 12:52:57 CEST 2026                                                */

// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace juggluco::activity::mirror {
// backup.dat has four-byte alignment. Keep the original cursor's member offsets
// without adding eight-byte alignment or an unused word to changednums.
#pragma pack(push,4)
struct Cursor {uint64_t epoch=0,revision=0;uint32_t dataSize=0;};
#pragma pack(pop)
static_assert(sizeof(Cursor)==20&&alignof(Cursor)==4);
static_assert(offsetof(Cursor,epoch)==0&&offsetof(Cursor,revision)==8&&offsetof(Cursor,dataSize)==16);
static_assert(std::is_standard_layout_v<Cursor>&&std::is_trivially_copyable_v<Cursor>);
}
