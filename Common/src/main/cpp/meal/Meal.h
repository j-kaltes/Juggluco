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


#pragma once
#include "inout.h"
#include <array>
#include <vector>
#include <regex>
#include "datbackup.h"
extern Backup *backup;

extern std::string_view globalbasedir;

inline static constexpr const char mealsdat[]="meals.dat";
struct ingredient_t {
	int unit;
	int used;
	float carb;
	std::array<char,40> name;
	};
typedef std::array<ingredient_t,410> ingredients_t;
typedef std::array<char,20> ingredientunit_t;
typedef std::array<ingredientunit_t,40>  units_ingredient_t;
constexpr const int startsize=63488;
//	uint32_t lastmeal,lastingredient;
//	uint32_t gotlastmeal,gotlastingredient;
struct mealdata {
static inline int maxmeals=startsize;
        uint8_t unitnr;
	uint8_t reserved[3];
	uint16_t ingredientnr;
        uint16_t gotlastingredient;
	uint32_t gotlastmeal;
        uint32_t mealindex;
	units_ingredient_t units;
	ingredients_t ingredients;
	struct mealel {
		uint32_t ingr;
		union {
			float amount;
			uint32_t numindex;
			};
		};
	mealel   themeals[];

//Used data:
/*
0-offsetof(units[unitnr])
ingredients[lastingredient]-ingredients[ingredientnr]
themeals[lastmealindex]-themeals[mealindex+1] */
//void endmeal() { themeals[++mealindex].ingr=0; ++mealnr; }
bool deleteable(int pos)  {
	return (pos==(ingredientnr-1))&&!ingredients[pos].used;
	}
int getindex()  {
	LOGGER("getindex=%d\n",mealindex);
	themeals[mealindex].ingr=0; 
	return mealindex;
	}
int endmeal(const int index) {
	LOGGER("endmeal(%d) mealindex=%d\n",index,mealindex);
	if(index>mealindex) {
		mealindex=index+1;;
		}
	themeals[mealindex].ingr=0; 
	if(index==mealindex)
		return 0;
	return index;
	}





           

void searchingredients(const char *searchstr,std::vector<int> &hits) const {
  try {
	std::regex zoek(searchstr, std::regex_constants::icase);
	for(int i=0;i<ingredientnr;i++) {
	     if(std::regex_search(ingredients[i].name.data(),zoek))
	  	hits.push_back(i);
	  }	
    }
    catch (const std::regex_error& e) {
        	LOGGER( "exception std::regex %s\n", e.what());
        }
  }
bool matchmeals(const std::vector<int> &ingr,const float amount,const int mealptr) const {
	int nr=themeals[mealptr].ingr;	
	if(nr>mealptr)
		return false;
	int start=mealptr-nr;
	for(int i=start;i<mealptr;i++) {
		int in=themeals[i].ingr;
		for(int el:ingr) {	
			if(in==el&&themeals[i].amount>=amount)
				return true;
			}
		}
	return false;
	}
	/*
struct sendmeal  {
	uint16_t com;
         uint16_t  datalen;
	 uint32_t data[];
	 };
	 */


void updatelastmeal(int last) {
	const int hnr=backup->getsendhostnr();
	for(int i=0;i<hnr;i++) {
		struct changednums *nu=backup->getnums(i,0); 
		if(last<nu->lastmeal)
			nu->lastmeal=last;
		}
	}
int updatemeal( crypt_t*pass,int sock,uint32_t &lastmeal) {
	int startmeal=std::min(lastmeal,gotlastmeal);
	if(mealindex>startmeal) {
		std::vector<subdata> vect;
		vect.reserve(5);
		vect.push_back({reinterpret_cast<uint8_t*>(&startmeal),(int)offsetof(mealdata,gotlastmeal),(int)sizeof(gotlastmeal)});
		vect.push_back({reinterpret_cast<uint8_t*>(this),0,(int)offsetof(mealdata,gotlastingredient)});
		const int startind=offsetof(mealdata,mealindex);
		vect.push_back({reinterpret_cast<uint8_t*>(&mealindex),startind,(int)(offsetof(mealdata,units)-startind+unitnr*sizeof(units[0]))});
		vect.push_back({reinterpret_cast<uint8_t*>(&ingredients),(int)offsetof(mealdata,ingredients),(int)(ingredientnr*sizeof(ingredients[0]))});
		vect.push_back({reinterpret_cast<uint8_t*>(themeals+startmeal),(int)(offsetof(mealdata,themeals)+startmeal*sizeof(themeals[0])),(int)((mealindex-startmeal)*sizeof(themeals[0]))});
		if(!senddata(pass,sock,vect,mealsdat) ) {
			return 0;
			}
		lastmeal=mealindex;
		gotlastmeal=UINT32_MAX;
		//gotlastingredient=UINT16_MAX;
		return 1;	
		}
	return 2;
	}
	
void deletemeal(int index) {
	int nr=themeals[index].ingr;
	int start=index-nr;
	for(int i=start;i<index;i++) {
		--(ingredients[themeals[i].ingr].used);
		}
	if(mealindex<=(index+1)) {
		if(mealindex>start) 
			mealindex=start;
		}	
	else {
		for(int i=start;i<=index;i++)
			themeals[i].ingr=mealdeleted;
		}
	LOGGER("deletemeal(%d), mealindex=%d\n",index,mealindex);
	themeals[mealindex].ingr=0;
	updatelastmeal(start);
	}
//void zeromeal() { themeals[mealindex].ingr=0; }
static constexpr const uint32_t mealdeleted= UINT32_MAX;
//int additemtomeal(uint32_t ingre,float amount) { return additemtomeal(0,ingre,amount); }
int additemtomeal(int index,uint32_t ingre,float amount) {
//	int index=inindex<=0?mealindex:inindex;
	LOGGER("additemtomeal(%d,%d,%.1f) mealindex=%d\n",index,ingre,amount,mealindex);
	uint32_t nr=themeals[index].ingr;
	if(index<mealindex&&themeals[index+1].ingr!=mealdeleted) {
		int start=index-nr;
		memcpy(themeals+mealindex,themeals+start,sizeof(themeals[0])*nr);
		for(int i=start;i<=index;i++)
			themeals[i].ingr=mealdeleted;
		index=mealindex+nr;
		updatelastmeal(start);
		}
	else
		updatelastmeal(index);
	++(ingredients[ingre].used);
	themeals[index]={.ingr=ingre,.amount=amount};
	themeals[++index].ingr=nr+1;
	themeals[index].amount=NAN;
	return index;
	}
int changemealitem(int mindex,int pos,uint32_t ingre,float amount) {
	if(pos<0) {
		return additemtomeal(mindex, ingre, amount);
		}
	LOGGER("changemealitem(%d,%d,%d,%.1f) mealindex=%d\n",mindex,pos,ingre,amount,mealindex);
//	if(mindex<=0)  { mindex=mealindex; }
	uint32_t nr=themeals[mindex].ingr;
	int start=mindex-nr;
	int index=start+pos;

	updatelastmeal(index);
	--(ingredients[themeals[index].ingr].used);
	++(ingredients[ingre].used);
	themeals[index]={.ingr=ingre,.amount=amount};
	return mindex;
	}
int deletefrommeal(int mindex,int pos) {
	LOGGER("deletefrommeal(%d,%d)\n",mindex,pos);	
	uint32_t nr=themeals[mindex].ingr;
	themeals[mindex].ingr=mealdeleted;
	int start=mindex-- - nr--;
	const int firstpos= start+pos;
	struct mealel *first= themeals+firstpos;
	if(nr>pos)
		memmove(first,first+1,(nr-pos)*sizeof(themeals[0]));
	updatelastmeal(firstpos);
	themeals[mindex].ingr=nr;
	themeals[mindex].amount=NAN;
	return mindex;
	}

int cpmeal(int index) {
	LOGGER("cpmeal(%d)\n",index);	
	uint32_t nr=themeals[index].ingr;
	uint32_t start=index-nr;
	memcpy(themeals+mealindex,themeals+start,sizeof(themeals[0])*(nr+1));
	return mealindex+nr;
	}
//int itemsinmeal() const { return itemsinmeal(mealindex); }
bool goodmeal(const int index) const {
	const int nr=themeals[index].ingr;
	if(nr<40&&nr<=index&&isnan(themeals[index].amount)) {
		return true;
		}
	LOGGER("wrong meal index=%d nr=%d amount=%f\n",index,nr, themeals[index].amount);
	return false;
	}

int itemsinmeal(const int index) const {
//	if(index<=0) index=mealindex;
	const int nr=themeals[index].ingr;
	const auto rest=themeals[index].amount;
	if(nr<40&&nr<=index&&(!rest||isnan(rest)))
		return nr;
	return 0;
	}

float carbinmeal(int index) const {
//	if(index<=0) index=mealindex;
	if(index<0||index> maxmeals) //TODO remove
		return 0.0f;
	auto nr=itemsinmeal(index);
	int start=index-nr;
	float total=0.0f;
	for(int i=start;i<index;i++) {
		float amount=themeals[i].amount;
		int ingr= themeals[i].ingr;
		if(ingr<0||ingr>=ingredients.size())
			return 0.0f;
		float carb=ingredients[ingr].carb;
		total+=(amount*carb);
		}
	return total;
	}
int getitemingredient(int mindex,int pos) const {
//	if(mindex<=0) mindex=mealindex;
	int nr=itemsinmeal(mindex);
	
	if(pos<nr) {
		int start=mindex-nr;
		int index=start+pos;
		return themeals[index].ingr;
		}
	else 
		return -1;
	}
	
const char* getitemingredientname(const int mindex,const int pos) const {
//	if(mindex<=0) mindex=mealindex;
	int nr=itemsinmeal(mindex);
	if(pos<nr) {
		const int start=mindex-nr;
		const int index=start+pos;
		const int ingrnr=themeals[index].ingr;
		if(ingrnr<ingredientnr)
			return ingredients[ingrnr].name.data();
		else {
			LOGGER("getitemingredientname ingrnr(%d)>= ingredientnr (%d)\n",ingrnr,ingredientnr);
			return nullptr;
			}
		}
	else  {
		LOGGER("getitemingredientname pos(%d)  > nr(%d)\n",pos,nr);
		return nullptr;
		}
	}
//const char* getitemingredientname(const int pos) const { return getitemingredientname(0,pos);
/*
	int nr=themeals[mealindex].ingr;
	if(pos<nr) {
		int index=mealindex-pos-1;
		return ingredients[themeals[index].ingr].name.data();
		}
	else 
		return nullptr;
	}*/
float getitemamount(int mindex,const int pos) const {
//	if(mindex<=0) mindex=mealindex;
	//int nr=themeals[mindex].ingr;
	int nr=itemsinmeal(mindex);
	if(pos<nr) {
		int start=mindex-nr;
		int index=start+pos;
		return themeals[index].amount;
		}
	else 
		return NAN;
	}
//float getitemamount(const int pos) const { return  getitemamount(0,pos) ; }
};

class Meal: public Mmap<uint32_t> {
public:
	Meal():Mmap(globalbasedir,mealsdat,startsize) {
		}
	mealdata *datameal() {
		return reinterpret_cast<mealdata *>(Mmap::data());
		}
	const mealdata *datameal() const {
		return reinterpret_cast<const mealdata *>(Mmap::data());
		}
ingredients_t &getingredients() {
	return datameal()->ingredients;
	}
units_ingredient_t &getunits() {
	return datameal()->units;
	}

int totmeals() const {
	constexpr const	int elsize= sizeof(unittype);
	const int totsize=size()*elsize;
	const int formeals=totsize-offsetof(mealdata,themeals) ;
	mealdata::maxmeals=formeals/sizeof(datameal()->themeals[0]);
	LOGGER("size=%d, elsize=%d formeals=%d maxmeals=%d\n",totsize,elsize,formeals,mealdata::maxmeals);
	return mealdata::maxmeals;
	}
int shouldextend() const {
	const int tot=totmeals();
	if(datameal()->mealindex>(tot*.8)) {
		const int extendwith= (datameal()->mealindex*sizeof(datameal()->themeals[0]))/sizeof(unittype);
		LOGGER("should extend with %d\n",extendwith);
		return extendwith;
		}
		
	else {
		LOGGER("should not extend\n");
		return 0;
		}
	}

};
struct meal {
	const struct mealdata *data;
	const	int mealptr,start;
   meal(const struct mealdata *data,const uint32_t mealptr):data(data),mealptr(mealptr),start(mealptr-size()) {
	}
int size() const {
	return data->itemsinmeal(mealptr) ;
	}
float quantity(const int pos) const {
	return data->themeals[pos+start].amount;
	}
bool valid(const int pos) const {
	int ingr=data->themeals[pos+start].ingr;
	if(ingr>=0&&ingr<data->ingredientnr)
		return true;
	return false;
	}
const struct ingredient_t *ingredient(const int pos) const {
	return &data->ingredients[data->themeals[pos+start].ingr];
	}

};
