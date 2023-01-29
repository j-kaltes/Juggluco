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


#include <stdio.h>
#include <vector>
#include <jni.h>
#include "inout.h"

#define abbottdec(x) (*x)
#define abbottcall(x) x
#define WAS_JNIEXPORT 
WAS_JNIEXPORT jint JNICALL   abbottdec(P1)(JNIEnv *envin, jobject obj, jint i, jint i2,jbyteArray  bArr,jbyteArray  bArr2) ;
WAS_JNIEXPORT jbyteArray JNICALL abbottdec(P2)(JNIEnv *, jobject, jint, jint, jbyteArray, jbyteArray);
struct pstruct {
	int i1; 
	int i2;
	std::vector<const unsigned char> ar1;
	std::vector<const unsigned char> ar2;
	virtual void print(FILE *fp)=0;
	pstruct(int i1,int i2, std::vector<const unsigned char> &&ar1, std::vector<const unsigned char> &&ar2): i1(i1),i2(i2),ar1(std::move(ar1)),ar2(std::move(ar2)) {};
	virtual bool test(JNIEnv *env)=0;
	};
struct p1struct:pstruct {
	int res;
	p1struct(int i1,int i2, std::vector<const unsigned char> &&ar1, std::vector<const unsigned char> &&ar2,int res): pstruct(i1,i2,std::move(ar1),std::move(ar2)),res(res) {};
	virtual void print(FILE *fp) override {
		fprintf(fp,"P1(%d,%d,{",i1,i2);
		for(auto el:ar1) 
			fprintf(fp,"0x%x,",el);
		fprintf(fp,"},{");
		for(auto el:ar2) 
			fprintf(fp,"0x%x,",el);
		fprintf(fp,"})=%d\n",res);
			
		}
	bool test(JNIEnv *env) override {	
		data_t *dar1=ar1.size()?data_t::newex(ar1):nullptr;
		data_t *dar2=ar2.size()?data_t::newex(ar2):nullptr;
		int nores=abbottcall(P1)(env,nullptr,i1,i2,reinterpret_cast<jbyteArray>(dar1),reinterpret_cast<jbyteArray>(dar2));
		return nores==res;
		}
	}; 
struct p2struct:pstruct {
	std::vector<const unsigned char> res;
	p2struct(int i1,int i2, std::vector<const unsigned char> &&ar1, std::vector<const unsigned char> &&ar2,std::vector<const unsigned char> && res): pstruct(i1,i2,std::move(ar1),std::move(ar2)),res(std::move(res)) {};
	
	virtual  void print(FILE *fp) override {
		fprintf(fp,"P2(%d,%d,{",i1,i2);
		for(auto el:ar1) 
			fprintf(fp,"0x%x,",el);
		fprintf(fp,"},{");
		for(auto el:ar2) 
			fprintf(fp,"0x%x,",el);
		fprintf(fp,"})={");
		for(auto el:res) 
			fprintf(fp,"0x%x,",el);
		fprintf(fp,"}};\n");
		}
	bool test(JNIEnv *env) override {	
		data_t *dar1=ar1.size()?data_t::newex(ar1):nullptr;
		data_t *dar2=ar2.size()?data_t::newex(ar2):nullptr;
		data_t *nores=reinterpret_cast<data_t*>(abbottcall(P2)(env,nullptr,i1,i2,reinterpret_cast<jbyteArray>(dar1),reinterpret_cast<jbyteArray>(dar2)));
		return nores->size()==res.size()&&!memcmp(nores->data(),res.data(),res.size());
		}
	}; 
#include "funcs2.h"

#include <iostream>
using std::cout;
using std::endl;
int main() {
std::vector<pstruct*> alg{&p1_0,&p2_0,&p1_1,&p2_1,&p1_2};

	for(auto el:alg) {
		el->print(stdout);
		}
	p1_0.print(stdout);
	p2_0.print(stdout);
	}
