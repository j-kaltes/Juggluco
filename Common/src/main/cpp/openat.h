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


constexpr const std::string_view opennames[] = { 
#if defined(__aarch64__) ||  defined(__arm__) || defined(__i386__)
"/data/dalvik-cache/" archname "/system@framework@" archname "@boot-framework.oat@xposed",
"/data/dalvik-cache/oat/" archname "/xposed_XTypedArraySuperClass.odex",
#else
"/data/dalvik-cache/oat/" archname "/xposed_XTypedArraySuperClass.odex",
"/data/dalvik-cache/" archname "/system@framework@" archname "@boot-framework.oat@xposed",
#endif
"/data/dalvik-cache/xposed_XResourcesSuperClass.dex",
"/root/magisk",
"/sbin/amphoras",
"/sbin/daemonsu",
"/sbin/su",
"/su/bin/daemonsu",
"/su/bin/su",
"/system/bin/amphoras",
"/system/bin/daemonsu",
"/system/bin/su",
"/system/framework/XposedBridge.jar",
"/system/sbin/amphoras",
"/system/sbin/daemonsu",
"/system/sbin/su",
"/system/xbin/bstk/su",
"/system/xbin/daemonsu",
"/system/xbin/su"};
