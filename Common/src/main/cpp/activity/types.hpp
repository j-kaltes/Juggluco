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
#include <cstdint>
#include <string>

namespace juggluco::activity {
// Stable ExerciseSessionRecord codes, connect-client 1.1.0-rc03. Keep the
// numeric type in exports too, including types added by newer Health Connect.
inline std::string typeName(uint32_t type) {
    switch(type) {
    case 0:return "Other workout";
    case 2:return "Badminton";
    case 4:return "Baseball";
    case 5:return "Basketball";
    case 8:return "Biking";
    case 9:return "Biking stationary";
    case 10:return "Boot camp";
    case 11:return "Boxing";
    case 13:return "Calisthenics";
    case 14:return "Cricket";
    case 16:return "Dancing";
    case 25:return "Elliptical";
    case 26:return "Exercise class";
    case 27:return "Fencing";
    case 28:return "Football american";
    case 29:return "Football australian";
    case 31:return "Frisbee disc";
    case 32:return "Golf";
    case 33:return "Guided breathing";
    case 34:return "Gymnastics";
    case 35:return "Handball";
    case 36:return "High intensity interval training";
    case 37:return "Hiking";
    case 38:return "Ice hockey";
    case 39:return "Ice skating";
    case 44:return "Martial arts";
    case 46:return "Paddling";
    case 47:return "Paragliding";
    case 48:return "Pilates";
    case 50:return "Racquetball";
    case 51:return "Rock climbing";
    case 52:return "Roller hockey";
    case 53:return "Rowing";
    case 54:return "Rowing machine";
    case 55:return "Rugby";
    case 56:return "Running";
    case 57:return "Running treadmill";
    case 58:return "Sailing";
    case 59:return "Scuba diving";
    case 60:return "Skating";
    case 61:return "Skiing";
    case 62:return "Snowboarding";
    case 63:return "Snowshoeing";
    case 64:return "Soccer";
    case 65:return "Softball";
    case 66:return "Squash";
    case 68:return "Stair climbing";
    case 69:return "Stair climbing machine";
    case 70:return "Strength training";
    case 71:return "Stretching";
    case 72:return "Surfing";
    case 73:return "Swimming open water";
    case 74:return "Swimming pool";
    case 75:return "Table tennis";
    case 76:return "Tennis";
    case 78:return "Volleyball";
    case 79:return "Walking";
    case 80:return "Water polo";
    case 81:return "Weightlifting";
    case 82:return "Wheelchair";
    case 83:return "Yoga";
    default:return "Activity "+std::to_string(type);
    }
}
}
