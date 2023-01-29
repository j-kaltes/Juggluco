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
/*      Fri Jan 27 12:36:20 CET 2023                                                 */


#ifdef USE_MEAL
#include <algorithm>
#include <string.h>
#include "Meal.h"
#include "fromjava.h"

extern Meal *meals;
extern "C" JNIEXPORT void JNICALL fromjava(saveingredient)(JNIEnv *env, jclass thiz,jint pos, jstring jname,jstring junit,jfloat carbo) {
	ingredients_t &ingr=meals->getingredients();
	if(pos<0) {
		pos=meals->datameal()->ingredientnr++;
		}
	ingredient_t &add=ingr[pos];

	{jint jnamelen = env->GetStringLength( jname);
	jint rawlen = env->GetStringUTFLength( jname);
	int maxnamelen=add.name.size()-1;
	if(rawlen<=maxnamelen||(jnamelen==rawlen&&(jnamelen=maxnamelen))) {
		env->GetStringUTFRegion(jname, 0,jnamelen, add.name.data());
		add.name.data()[jnamelen]='\0';
		}
	else {
		char tmpbuf[rawlen];
		env->GetStringUTFRegion(jname, 0,jnamelen, tmpbuf);
		memcpy(add.name.data(),tmpbuf,maxnamelen);
		add.name.data()[maxnamelen]='\0';
		}
	}
	units_ingredient_t &units=meals->getunits();
	int unitnr= meals->datameal()->unitnr;
	ingredientunit_t &unit=units[unitnr];
	jint junitlen = env->GetStringLength( junit);
	jint rawunitlen = env->GetStringUTFLength( jname);
	const int maxunitlen=unit.size()-1;	
	char *newunit= unit.data();
	if(junitlen<=maxunitlen||(rawunitlen==junitlen&&(junitlen=maxunitlen))) {
		env->GetStringUTFRegion(junit, 0,junitlen,newunit);
		newunit[junitlen]='\0';
		}
	else {
		char tmpbuf[rawunitlen];
		env->GetStringUTFRegion(junit, 0,junitlen,tmpbuf);
		memcpy(newunit,tmpbuf,maxunitlen);
		newunit[maxunitlen]='\0';
		}
	auto unit_it=std::find_if(units.begin(),&units[unitnr],[newunit](ingredientunit_t &one) {
		return !strcasecmp(one.data(),newunit) ;
		});
	int index=unit_it-units.begin();
	if(index==meals->datameal()->unitnr) {
		++(meals->datameal()->unitnr);
		}
	add.unit=index;
	add.carb=carbo;
	}
extern "C" JNIEXPORT jboolean JNICALL fromjava(ingredientdeleteable)(JNIEnv *env, jclass thiz,jint pos) {
	return meals->datameal()->deleteable(pos);
	}
extern "C" JNIEXPORT void JNICALL fromjava(deleteingredient)(JNIEnv *env, jclass thiz,jint pos) {
	if((meals->datameal()->ingredientnr-1)==pos)
		--(meals->datameal()->ingredientnr);
/*
	if(--meals->datameal()->ingredientnr>pos) {
		memmove(&meals->getingredients()[pos],&meals->getingredients()[pos+1],sizeof(meals->getingredients()[0])*(meals->datameal()->ingredientnr-pos));
		} */
	
	}




extern "C" JNIEXPORT jint JNICALL fromjava(ingredientNr)(JNIEnv *env, jclass thiz) {
	return meals->datameal()->ingredientnr;
	}
extern "C" JNIEXPORT jfloat JNICALL fromjava(ingredientCarb)(JNIEnv *env, jclass thiz,jint index) {
	return meals->getingredients()[index].carb;
	}
extern "C" JNIEXPORT jint JNICALL fromjava(ingredientUsed)(JNIEnv *env, jclass thiz,jint index) {
	return meals->getingredients()[index].used;
	}
extern "C" JNIEXPORT jstring JNICALL fromjava(ingredientUnitName)(JNIEnv *env, jclass thiz,jint index) {
	return env->NewStringUTF(meals->getunits()[meals->getingredients()[index].unit].data());
	}
extern "C" JNIEXPORT jint JNICALL fromjava(ingredientUnit)(JNIEnv *env, jclass thiz,jint index) {
	return meals->getingredients()[index].unit;
	}
extern "C" JNIEXPORT jstring JNICALL fromjava(ingredientName)(JNIEnv *env, jclass thiz,jint index) {
	return env->NewStringUTF(meals->getingredients()[index].name.data());
	}


extern "C" JNIEXPORT jint JNICALL fromjava(changemealitem)(JNIEnv *env, jclass thiz,jint mealptr,jint pos,jint ingre,jfloat amount) {
	return meals->datameal()->changemealitem(mealptr, pos,ingre,amount);
	}

extern "C" JNIEXPORT jint JNICALL fromjava(additemtomeal)(JNIEnv *env, jclass thiz,jint mealptr,jint ingre,jfloat amount) {
	return meals->datameal()->additemtomeal(mealptr,ingre,amount);
	}
extern "C" JNIEXPORT jfloat JNICALL fromjava(getitemamount)(JNIEnv *env, jclass thiz,jint mealptr,jint pos) {
	return meals->datameal()->getitemamount(mealptr,pos);
	}
extern "C" JNIEXPORT jstring JNICALL fromjava(getitemingredientname)(JNIEnv *env, jclass thiz,jint mealptr,jint pos) {
	const char * ingr=meals->datameal()->getitemingredientname(mealptr,pos);
	if(ingr)
		return env->NewStringUTF(ingr);
	return nullptr;
	}
extern "C" JNIEXPORT jint JNICALL fromjava(getitemingredient)(JNIEnv *env, jclass thiz,jint mealptr,jint pos) {
	return meals->datameal()->getitemingredient(mealptr,pos);
	}
extern "C" JNIEXPORT jint JNICALL fromjava(getmealitemnr)(JNIEnv *env, jclass thiz,jint mealptr) {
	return meals->datameal()->itemsinmeal(mealptr) ;
	}
extern "C" JNIEXPORT jint JNICALL fromjava(cpmeal)(JNIEnv *env, jclass thiz,jint mealptr) {
	return meals->datameal()->cpmeal(mealptr); 
	}
extern "C" JNIEXPORT void JNICALL fromjava(deletemeal)(JNIEnv *env, jclass thiz,jint mealptr) {
	  meals->datameal()-> deletemeal(mealptr);
	 }
extern "C" JNIEXPORT jfloat JNICALL fromjava(carbinmeal)(JNIEnv *env, jclass thiz,jint mealptr) {
	return meals->datameal()->carbinmeal(mealptr);
	}
extern "C" JNIEXPORT jint JNICALL fromjava(getnewmealptr)(JNIEnv *env, jclass thiz) {
	if(const int with=meals->shouldextend()) {
		meals->extend(globalbasedir,mealsdat,meals->size()+with);
		}
	return meals->datameal()-> getindex(); 
	}
extern "C" JNIEXPORT void JNICALL fromjava(closemeal)(JNIEnv *env, jclass thiz,jint mealptr) {
	 meals->datameal()-> endmeal(mealptr); 
	 }
extern "C" JNIEXPORT jint JNICALL fromjava(deletefrommeal)(JNIEnv *env, jclass thiz,jint mealptr,jint pos) {
	 return meals->datameal()-> deletefrommeal(mealptr,pos);
	 }

extern "C" JNIEXPORT jobject  JNICALL   fromjava(getunits)(JNIEnv *env, jclass cl) {
	int len= meals->datameal()->unitnr;
	jclass carlist=env->FindClass("java/util/ArrayList");
	jmethodID init=env->GetMethodID(carlist,"<init>","(I)V");
	jobject arlist=env->NewObject(carlist,init,len+1);
	jmethodID add=env->GetMethodID(carlist,"add","(Ljava/lang/Object;)Z");
	env->DeleteLocalRef(carlist);
	env->CallBooleanMethod(arlist,add, env->NewStringUTF( ""));
	 for(int i=0;i<len;i++) {
	 	    env->CallBooleanMethod(arlist,add, env->NewStringUTF( meals->datameal()->units[i].data()));
		  }
	return arlist;
	}
	 /*
extern "C" JNIEXPORT void JNICALL fromjava(zeromeal)(JNIEnv *env, jclass thiz) {
	 meals->datameal()-> zeromeal(); 
	 }*/

#endif

