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


namespace envirvar {
//static char var0[]="PATH=" BASEDIR "/bin";
static char var0[]="PATH=/system/bin";
static char var1[]="DOWNLOAD_CACHE=/data/cache";
static char var2[]="ANDROID_BOOTLOGO=1";
static char var3[]="ANDROID_ROOT=/system";
static char var4[]="ANDROID_ASSETS=/system/app";
static char var5[]="ANDROID_DATA=/data";
static char var6[]="ANDROID_STORAGE=/storage";
static char var7[]="EXTERNAL_STORAGE=/sdcard";
static char var8[]="ASEC_MOUNTPOINT=/mnt/asec";
static char var9[]="BOOTCLASSPATH=/system/framework/core-oj.jar:/system/framework/core-libart.jar:/system/framework/conscrypt.jar:/system/framework/okhttp.jar:/system/framework/legacy-test.jar:/system/framework/bouncycastle.jar:/system/framework/ext.jar:/system/framework/framework.jar:/system/framework/telephony-common.jar:/system/framework/voip-common.jar:/system/framework/ims-common.jar:/system/framework/apache-xml.jar:/system/framework/org.apache.http.legacy.boot.jar:/system/framework/android.hidl.base-V1.0-java.jar:/system/framework/android.hidl.manager-V1.0-java.jar:/system/framework/com.nxp.nfc.jar:/system/framework/mediatek-common.jar:/system/framework/mediatek-framework.jar:/system/framework/mediatek-telephony-common.jar:/system/framework/mediatek-telephony-base.jar:/system/framework/mediatek-ims-common.jar:/system/framework/mediatek-telecom-common.jar";
static char var10[]="SYSTEMSERVERCLASSPATH=/system/framework/services.jar:/system/framework/ethernet-service.jar:/system/framework/wifi-service.jar";
static char var11[]="MC_AUTH_TOKEN_PATH=/efs";
static char var12[]="ANDROID_SOCKET_zygote=8";
};
char *envptrs[]={envirvar::var0,envirvar::var1,envirvar::var2,envirvar::var3,envirvar::var4,envirvar::var5,envirvar::var6,envirvar::var7,envirvar::var8,envirvar::var9,envirvar::var10,envirvar::var11,envirvar::var12,NULL,NULL};
