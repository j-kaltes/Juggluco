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
/*      Fri Jan 27 12:39:13 CET 2023                                                 */


#include <jni.h>
#include <cstdio>
#include <cstring>
#include "settings/settings.hpp"
#include "settings/mixpass.hpp"

jbyteArray  getlibre3puttext(JNIEnv *env, const char *sensorname) {
	LOGGER("getlibre3puttext(%s)\n",sensorname);
static const char puttextstart[]=R"({
  "DateOfBirth": "%d",
  "DomainData": "{\n \"activeSensor\" : \"%s\"\n}",
  "FirstName": "%s",
  "GuardianFirstName": "%s",
  "GuardianLastName": "%s",
  "LastName": "%s",
  "UserToken": ")";
static const char puttextend[]=R"(",
  "Domain": "Libreview",
  "GatewayType": "FSLibreLink3.Android"
})";

const int tokenlen=settings->data()->tokensize3;
char puttext[512+tokenlen];
int endpos=sprintf(puttext,puttextstart,
		settings->data()->DateOfBirth,sensorname,
		getFirstName().data(),
		getGuardianFirstName().data(),
		getGuardianLastName().data(),
		getLastName().data());
	memcpy(puttext+endpos,settings->data()->libreviewUserToken3,tokenlen);
	endpos+=tokenlen;
	int endlen=sizeof(puttextend)-1;
	memcpy(puttext+endpos,puttextend,endlen);
	endpos+=endlen;
	logwriter(puttext,endpos);
	jbyteArray uit=env->NewByteArray(endpos);
        env->SetByteArrayRegion(uit, 0, endpos,(const jbyte *)puttext);
	return uit;
	}
#ifdef NOLOG
#define EXTRA 50
#else
#define EXTRA 0
#endif
jbyteArray getlibre2puttext(JNIEnv *env, const char *sensorname) {
	LOGGER("getlibre2puttext(%s)\n",sensorname);
	static constexpr const char putlibre2start[]=R"({"DomainData":"{\"activeSensor\":\"%s\"}","UserToken":")";
	static constexpr const char putlibre2end[]=R"(","Domain":"Libreview","GatewayType":"FSLibreLink.Android"})";

	const int tokenlen=settings->data()->tokensize;
	char puttext[sizeof(putlibre2start)+ sizeof(putlibre2end)+tokenlen+16-1+EXTRA];
	int endpos=sprintf(puttext, putlibre2start,sensorname);
	memcpy(puttext+endpos,settings->data()->libreviewUserToken,tokenlen);
	endpos+=tokenlen;
	constexpr const int endlen=sizeof(putlibre2end)-1;
	memcpy(puttext+endpos, putlibre2end,endlen);
	endpos+=endlen;
	logwriter(puttext,endpos);
	jbyteArray uit=env->NewByteArray(endpos);
        env->SetByteArrayRegion(uit, 0, endpos,(const jbyte *)puttext);
	return uit;
	}
