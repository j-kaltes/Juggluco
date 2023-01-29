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
/*      Fri Jan 27 15:21:16 CET 2023                                                 */


#include <jni.h>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <tinycrypt/ccm_mode.h>
#include "../share/fromjava.h"

#include "logs.h"
//#include <tinycrypt/constants.h>
typedef uint8_t byte;
static   constexpr const byte packetDescriptor[][3]={
            {(byte)0x00,(byte)0x00,(byte)0x00},
            {(byte)0x00,(byte)0x00,(byte)0x0F},
            {(byte)0x00,(byte)0x00,(byte)0xF0},
            {(byte)0x00,(byte)0x0F,(byte)0x00},
            {(byte)0x00,(byte)0xF0,(byte)0x00},
            {(byte)0x0F,(byte)0x00,(byte)0x00},
            {(byte)0xF0,(byte)0x00,(byte)0x00},
            {(byte)0x44,(byte)0x00,(byte)0x00}};
struct bcrypt {
static constexpr const int noncelen=13;
static constexpr const int taglen=4;
static constexpr const int iv_enc_len=8;
	int outCryptoSequence;
//	uint8_t nonce[noncelen];
	uint8_t iv_enc[iv_enc_len];
	uint8_t key[16];
struct nonce_t {
	uint8_t data[noncelen];
	nonce_t(uint8_t *iv_enc) {
		memcpy(data+5,iv_enc,iv_enc_len);
		}

	operator uint8_t *() {
		return data;
		}
	

	};
//s/byte\[\] \(\<[a-z]*\>\)/byte \1\[\]/g
    bool selfencrypt(byte input[],int inputlen,byte key[],byte nonce[],byte encrypted[]) {
	struct tc_ccm_mode_struct c;
	struct tc_aes_key_sched_struct sched;
	tc_aes128_set_encrypt_key(&sched, key);
	int result = tc_ccm_config(&c, &sched, nonce, noncelen, taglen);
	if(result == 0) {
		LOGGER("tc_ccm_config failed\n");
		return false;
		}
	int encrlen=inputlen+4;
	result = tc_ccm_generation_encryption(encrypted,encrlen ,nullptr, 0, input, inputlen, &c);
	if(result == 0) {
		LOGGER("tc_ccm_generation_encryption failed\n");
		return false;
		}
	return true;
	}
	bool	intEncrypt(int kind,uint8_t *plain,int inlen,uint8_t *encrypted) {
		int mod=      outCryptoSequence; //outCryptoSequence is set at 1 on program start and incremented here
		nonce_t nonceb(iv_enc);
		auto nonce=nonceb.data;
		nonce[0]= (byte)(mod&0xFF);
		nonce[1]=(byte)(0xFF&(mod>>8));
		memcpy(nonce+2, packetDescriptor[kind],3);
        	bool res=selfencrypt(plain, inlen,key, nonce,encrypted);
		int encryptlen=inlen+4;
		encrypted[encryptlen]=nonce[0];
		encrypted[encryptlen+1]=nonce[1];
		++outCryptoSequence;
		return res;
		}
 static bool selfdecrypt(byte encrypted[], int encryplen,byte key[], byte nonce[],byte plain[]) {
	struct tc_ccm_mode_struct c;
	struct tc_aes_key_sched_struct sched;
	tc_aes128_set_encrypt_key(&sched, key);
	int result = tc_ccm_config(&c, &sched, nonce, noncelen, taglen);
	if(result == 0) {
		LOGGER("tc_ccm_config failed\n");
		return false;
		}
	const int decryptlen=encryplen-4;
	result = tc_ccm_decryption_verification(plain,decryptlen , nullptr, 0, encrypted, encryplen, &c);
	if(result == 0) {
		LOGGER("tc_ccm_decryption_verification failed\n");
		return false;
		}
	return true;	
    }
	bool intDecrypt(int kind, uint8_t *encrypted, int inputlen,uint8_t *decrypted) {
		LOGGER("intDecrypt(%d,%p,%d,%p)\n",kind,encrypted,inputlen,decrypted);
		nonce_t nonceb(iv_enc);
		uint8_t *nonce=nonceb.data;
		int encryptlen=inputlen-2;
		memcpy(nonce,encrypted+encryptlen,2);
		memcpy(nonce+2,packetDescriptor[kind],3);
        	return selfdecrypt(encrypted,encryptlen,key,nonce,decrypted);
		}
	};
extern "C" JNIEXPORT jlong JNICALL fromjava(initcrypt)(JNIEnv *env, jclass thiz,jlong jprev,jbyteArray jkey,jbyteArray jiv) {
	bcrypt *ptr =reinterpret_cast<bcrypt *>(jprev);
	if(!ptr) ptr=new bcrypt;
	env->GetByteArrayRegion(jkey,0,16,(jbyte *)ptr->key);
	env->GetByteArrayRegion(jiv,0,8,(jbyte*)(ptr->iv_enc));
	ptr->outCryptoSequence=1;
	return reinterpret_cast<jlong>(ptr);
    };
extern "C" JNIEXPORT void JNICALL fromjava(endcrypt)(JNIEnv *env, jclass thiz,jlong jptr) {
	bcrypt *ptr=reinterpret_cast<bcrypt *>(jptr);
	delete ptr;
	}

//    	public byte[] fromjava(intEncrypt)(jint i2, byte[] plain) {
extern "C" JNIEXPORT jbyteArray JNICALL fromjava(intEncrypt)(JNIEnv *env, jclass thiz,jlong jcryptptr, jint kind,jbyteArray jplain) {
	if(!jcryptptr)
		return nullptr;
	if(!jplain)
		return nullptr;
	jsize inlen=env->GetArrayLength(jplain);
	if(inlen<=0)
		return nullptr;
	jsize uitlen= inlen+6;
	uint8_t plain[inlen];
	uint8_t encrypted[uitlen];
	env->GetByteArrayRegion(jplain,0,inlen,(jbyte*)plain);
	bcrypt *cryptptr=reinterpret_cast<bcrypt*>(jcryptptr);
	if(!cryptptr->intEncrypt(kind,plain,inlen,encrypted))
		return nullptr;
	jbyteArray jencrypt=env->NewByteArray(uitlen);
	env->SetByteArrayRegion(jencrypt, 0, uitlen, (jbyte*)encrypted);
	return jencrypt;
    	}
extern "C" JNIEXPORT jbyteArray JNICALL fromjava(intDecrypt)(JNIEnv *env, jclass thiz,jlong jcryptptr, jint kind,jbyteArray jencrypted) {
	if(!jcryptptr)
		return nullptr;
	if(!jencrypted)
		return nullptr;
	jsize encryptlen=env->GetArrayLength(jencrypted);
	if(encryptlen<7)
		return nullptr;
	jsize plainlen= encryptlen-6;
	uint8_t encrypted[encryptlen];
	uint8_t plain[plainlen];
	env->GetByteArrayRegion(jencrypted,0,encryptlen,(jbyte*)encrypted);
	bcrypt *cryptptr=reinterpret_cast<bcrypt*>(jcryptptr);
	if(!cryptptr->intDecrypt(kind,encrypted,encryptlen,plain))
		return nullptr;
	jbyteArray jplain=env->NewByteArray(plainlen);
	env->SetByteArrayRegion(jplain, 0, plainlen, (jbyte*)plain);
	return jplain;
    	}

    	

