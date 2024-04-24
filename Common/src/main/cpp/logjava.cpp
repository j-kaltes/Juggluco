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
/*      Fri Jan 27 12:35:35 CET 2023                                                 */


#include "logs.h"
#include "fromjava.h"
#include "settings/settings.h"

#ifndef NOLOG 
extern bool dolog;
#endif
extern "C" JNIEXPORT void JNICALL fromjava(log)(JNIEnv *env, jclass thiz,jstring jmess) {
#ifndef NOLOG 
	if(dolog) {
		const jint len = env->GetStringUTFLength( jmess);
		char mess[len+1];
		jint jlen = env->GetStringLength( jmess);
		env->GetStringUTFRegion(jmess, 0,jlen, mess);
		mess[len]='\0';
		LOGGERNO(mess,len,false);
		}
#endif
	}
extern "C"  JNIEXPORT void JNICALL fromjava(dolog)(JNIEnv *envin, jclass thiz,jboolean val) {
#ifndef NOLOG 
	if(settings) {
		settings->data()->nolog=!val;
		}
	dolog=val;
#endif
	}
extern "C"  JNIEXPORT jboolean JNICALL fromjava(islogging)(JNIEnv *env, jclass thiz) {
#ifndef NOLOG
	return dolog;
#else
	return 0;
#endif
	}

/*
extern "C" JNIEXPORT void JNICALL fromjava(showbytes)(JNIEnv *env, jclass thiz, jstring jstr,jbyteArray jbytes) {
	if(jstr) {
		const char *str = env->GetStringUTFChars( jstr, NULL);
		if (str == nullptr) return ;
		destruct   dest([jstr,str,env]() {env->ReleaseStringUTFChars(jstr, str);});
		if(jbytes) {
			const jsize lens=env->GetArrayLength(jbytes);
			uint8_t bytes[lens];
			env->GetByteArrayRegion(jbytes, 0, lens, (jbyte *)bytes);
			hexstr hex(bytes,lens);
			LOGGER("%s: %s\n",str,hex.str());
			}
		else
			LOGGER("%s zero bytes array\n",str);
		}
	} */


extern "C" JNIEXPORT void JNICALL fromjava(showbytes)(JNIEnv *env, jclass thiz,jstring jmess,jbyteArray jar) {
#ifndef NOLOG
	if(dolog) {
		const jint len = env->GetStringUTFLength(jmess);
		

		constexpr const char nullstr[]=" null";
		constexpr const int nulllen=sizeof(nullstr)-1;
        	const jsize vallen=jar?env->GetArrayLength(jar):0;

		constexpr const int startlen=0;
		const int totlen=startlen+len+(jar?(vallen*3):nulllen)+1;
		char mess[totlen];
		const jint jlen = env->GetStringLength(jmess);
		env->GetStringUTFRegion(jmess, 0,jlen, mess+startlen);
		int pos=startlen+len;
		if(jar) {
			if(vallen) {
				uint8_t value[vallen];
				env->GetByteArrayRegion(jar, 0, vallen,(jbyte *) value);
				for(int i=0;i<vallen;i++) {
					pos+=sprintf(mess+pos," %02X",value[i]);
					}
				}
			}
		else {
			memcpy(mess+pos,nullstr,nulllen);
			pos+=nulllen;
			}
		LOGGERN(mess,pos);
		}
#endif
        }


