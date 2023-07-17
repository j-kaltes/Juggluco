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

//R"(/data/dalvik-cache/)" archname R"(/system@framework@)" archname R"(@boot-framework.oat@xposed)"
constexpr const std::string_view opennames[] = { 
#if defined(__aarch64__)  || defined(__i386__)
R"(/data/dalvik-cache/)" archname R"(/system@framework@)" archname R"(@boot-framework.oat@xposed)",
R"(/data/dalvik-cache/oat/)" archname R"(/xposed_XTypedArraySuperClass.odex)",
#else
R"(/data/dalvik-cache/oat/)" archname R"(/xposed_XTypedArraySuperClass.odex)",
R"(/data/dalvik-cache/)" archname R"(/system@framework@" archname "@boot-framework.oat@xposed)",
#endif
R"(/data/dalvik-cache/xposed_XResourcesSuperClass.dex)",
R"(/root/magisk)",
R"(/sbin/amphoras)",
R"(/sbin/daemonsu)",
R"(/sbin/su)",
R"(/su/bin/daemonsu)",
R"(/su/bin/su)",
R"(/system/bin/amphoras)",
R"(/system/bin/daemonsu)",
R"(/system/bin/su)",
R"(/system/framework/XposedBridge.jar)",
R"(/system/lib/libriruloader.so)",
R"(/system/lib64/libriruloader.so)",
R"(/system/sbin/amphoras)",
R"(/system/sbin/daemonsu)",
R"(/system/sbin/su)",
R"(/system/xbin/bstk/su)",
R"(/system/xbin/daemonsu)",
R"(/system/xbin/su)"};
