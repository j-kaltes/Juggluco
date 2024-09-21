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
/*      Fri Jan 27 12:36:58 CET 2023                                                 */


#pragma once

#include <array>
#include "settings.hpp"
static	constexpr const uint8_t mixpass[]{0xD6,0x12,0x76,0xA9,0x4D,0xE4,0x09,0xA3,0xEF,0xBB,0xDB,0x21,0xA3,0x50,0xCD,0xD1,0x66,0x41,0x04,0x09,0xB5,0x74,0xEE,0x97,0x95,0x3C,0xB9,0x90,0x9F,0xF2,0x29,0x4D,0x65,0xEA,0xC4,0xE3,0x51,0x40,0x42,0x8D,0xC1};
inline void mix(const uint8_t *mixer,uint8_t *bufin,uint8_t *bufout,int buflen) {
	for(int i=0;i<buflen;i++)
		bufout[i]=bufin[i]^mixer[i];
	}
inline std::array<char,sizeof(settings->data()->LastName)> getLastName() {
        std::array<char,sizeof(settings->data()->LastName)> name;
        mix(mixpass, settings->data()->LastName,reinterpret_cast<uint8_t*>( name.data()), name.size());
        return name;
        }
inline std::array<char,sizeof(settings->data()->FirstName)> getFirstName() {
        std::array<char,sizeof(settings->data()->FirstName)> name;
        mix(mixpass, settings->data()->FirstName,reinterpret_cast<uint8_t*>( name.data()), name.size());
        return name;
        }
inline std::array<char,sizeof(settings->data()->GuardianFirstName)> getGuardianFirstName() {
        std::array<char,sizeof(settings->data()->GuardianFirstName)> name;
        mix(mixpass, settings->data()->GuardianFirstName,reinterpret_cast<uint8_t*>( name.data()), name.size());
        return name;
        }
inline std::array<char,sizeof(settings->data()->GuardianLastName)> getGuardianLastName() {
        std::array<char,sizeof(settings->data()->GuardianLastName)> name;
        mix(mixpass, settings->data()->GuardianLastName,reinterpret_cast<uint8_t*>( name.data()), name.size());
        return name;
        }
