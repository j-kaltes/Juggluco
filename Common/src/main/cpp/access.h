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


constexpr const std::string_view accessnames[] = {
#if defined(__aarch64__) ||  defined(__arm__) || defined(__i386__)
R"(/data/dalvik-cache/)" archname R"(/system@framework@XposedBridge.jar@classes.dex)",
R"(/data/dalvik-cache/oat/)" archname R"(/xposed_XResourcesSuperClass.odex)",
#else
R"(/data/dalvik-cache/oat/)" archname R"(/xposed_XResourcesSuperClass.odex)",
R"(/data/dalvik-cache/)" archname R"(/system@framework@XposedBridge.jar@classes.dex)",
#endif
R"(/data/dalvik-cache/xposed_XTypedArraySuperClass.dex)",
R"(/data/xposed/XposedBridge.jar)",
R"(/root/magisk)",
R"(/root/magiskinit)",
R"(/sbin/su)",
R"(/su/bin/daemonsu)",
R"(/su/bin/su)",
R"(/system/bin/app_process32_xposed)",
R"(/system/bin/su)",
R"(/system/lib64/libxposed_art.so)",
R"(/system/xbin/bstk/su)",
R"(/system/xbin/daemonsu)",
R"(/system/xbin/su)"};
