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


 #include <unistd.h>
 #include <string_view>
        #include <time.h>
#include "settings/settings.h"
#include "Meal.h"
#include "MealSave.h"

constexpr const char starthtml[]=R"(<!DOCTYPE html>
<html>
  <head>
    <meta http-equiv="content-type" content="text/html; charset=UTF-8">
    <title>Meals</title>
    <style>
* {
  box-sizing: border-box;
}

.row {
  display: flex;
  margin-left:-5px;
  margin-right:-5px;
}

.column {
  flex: 1%;
  padding: 5px;
}

table, td, th {
  border: 1px solid black;
}

table {
  border-collapse: collapse;
}
</style> </head>
  <body>)";
  constexpr const int startlen=sizeof(starthtml)-1;

constexpr const char starttable[]=R"(<div class="column"><table style="white-space:nowrap;">)";
//constexpr const char starttable[]=R"(<div class="column"><table><caption  style="text-align:left">)";
constexpr const int  starttablelen=sizeof(starttable)-1;

constexpr const char   endhead[]=R"(<col style="width:15ch">
        <col style="width:1ch">
        <col style="width:8ch">
      <thead style="background-color:#1E90FF;color:black;">
        <tr>
          <td>Ingredient</td>
          <td>Quantity</td>
          <td style="text-align:center">Unit</td>
          <td>Carb/unit</td>
          <td>Carbohydrate</td>
        </tr>
      </thead>
      <tbody>)";
constexpr const int endheadlen=sizeof(endhead)-1;

constexpr const  std::string_view startfoot(R"(</tbody>
      <tfoot>
        <tr>
          <td><b>)");
/*	  Total</b></td>
          <td colspan="3"></td>
          <td style="text-align:right"><b>)"); */

constexpr const  std::string_view date2total(R"(</b></td>

          <td ></td>
          <td><b>Total</b></td>
          <td ></td>
          <td style="text-align:right"><b>)");

constexpr const  std::string_view endtable(R"(</b></td>
        </tr>
      </tfoot>
    </table>
   </div>
    )");
constexpr const  std::string_view endhtml(R"(</div></body></html>)");
   


bool dostarthtml(FILE * handle) {
	return fwrite(starthtml,1,startlen,handle)==startlen;
	}


bool writeview(FILE * handle,std::string_view str) {
	return fwrite(str.data(),1,str.size(),handle)==str.size();
	}
#include "nums/num.h"
bool MealSave::dostarttable(FILE * handle,const Num * num) {
	const time_t dat=num->gettime();
      struct tm tmbuf;
      localtime_r(&dat, &tmbuf);
      if(was.different(tmbuf)) {
	  constexpr const std::string_view endandnewrow(R"(</div><div class="row">)");
	  constexpr const std::string_view newrow(R"(<div class="row">)");
      	if(was.year) {
		 if(!writeview(handle,endandnewrow))
		    return false;
		   }
		else {
		if(!writeview(handle,newrow))
		    return false;
		}
	was={tmbuf.tm_mday,tmbuf.tm_mon,tmbuf.tm_year};
      	  }

	if(fwrite(starttable,1,starttablelen,handle)!=starttablelen)
		return false;
		/*
	constexpr const int buflen=60;
	char buf[buflen];
	int len=snprintf(buf,buflen,"%g ",num->value);
	std::string_view lab=settings->getlabel(num->type);
	memcpy(buf+len,lab.data() ,lab.size());
	len+=lab.size();
//	int len=snprintf(buf,buflen,"%d-%02d-%02d-%02d:%02d",1900+tmbuf.tm_year,tmbuf.tm_mon+1,tmbuf.tm_mday,tmbuf.tm_hour,tmbuf.tm_min);
        len+=strftime(buf+len,buflen-len,R"( <b>%a %b %e %H:%M %Y</b>)",  &tmbuf);

	if(fwrite(buf,1,len,handle)!=len)
		return false; */
	return fwrite(endhead,1,endheadlen,handle)==endheadlen;
	}
#define writef(handle,data,size) fwrite(data,1,size,handle)

//std::string_view total2date=R"(</b></td></tr><tr><td><b>)";



bool doendtable(FILE* handle,float total,time_t dat) {
	if(writef(handle,startfoot.data(),startfoot.size())!=startfoot.size())
		return false;

{	constexpr const int buflen=60;
	char buf[buflen];
      struct tm tmbuf;
      localtime_r(&dat, &tmbuf);
       int tlen=snprintf(buf,buflen,"%04d-%02d-%02d %02d:%02d",  tmbuf.tm_year+1900,tmbuf.tm_mon+1,tmbuf.tm_mday,tmbuf.tm_hour,tmbuf.tm_min); 
       if(writef(handle,buf,tlen)!=tlen)
		return false;
}
	if(writef(handle,date2total.data(),date2total.size())!=date2total.size())
		return false;
 {	constexpr const int  buflen=10;
	char buf[buflen];
	int len=snprintf(buf,buflen,"%.1f",total);
	if(writef(handle,buf,len)!=len)
		return false;
}
	if(writef(handle,endtable.data(),endtable.size())!=endtable.size())
		return false;
	return true;
	}

bool writetabitem(FILE* handle,std::string_view item,const std::string_view td=R"(<td>)") {
	if(writef(handle,td.data(),td.size())!=td.size())
		return false;
	if(writef(handle,item.data(),item.size())!=item.size())
		return false;

	constexpr const char itemend[]="</td>\n";
	constexpr const int itemendlen=sizeof(itemend)-1;
	if(writef(handle,itemend,itemendlen)!=itemendlen)
		return false;
	return true;
	}
bool writetabnumitem(FILE* handle,std::string_view item) {
	return writetabitem(handle,item,R"(<td  style="text-align:right" >)");
	}
//savemeal(int handle,uint32_t date,const int index)  
bool MealSave::savemeal(FILE* handle,const Num *num)  {
	const int index=num->mealptr;
	extern Meal *meals;

	meal ameal(meals->datameal(),index);
	const int nr=ameal.size(); 
	if(nr<=0||nr>index)
		return true;// or ignore?
	if(!dostarttable(handle,num))
		return false;
	constexpr const char rowstart[]="<tr>";
	constexpr const int startlen=sizeof(rowstart)-1;
	constexpr const char rowend[]="</tr>";
	constexpr const int endlen=sizeof(rowend)-1;
	float totmeal=0.0f;
	for(int i=0;i<nr;i++) {
		if(!ameal.valid(i))
			 continue;
		if(writef(handle,rowstart,startlen)!=startlen)
			return false;
		 const ingredient_t *ingr=ameal.ingredient(i);
		if(!writetabitem(handle, ingr->name.data()))
			return false;
		const float quant=ameal.quantity(i);
		constexpr const int  buflen=10;
		char buf[buflen];
		int len=snprintf(buf,buflen,"%.1f",quant);
		if(!writetabnumitem(handle, std::string_view(buf,len)))
			return false;
		if(!writetabitem(handle,meals->datameal()->units[ingr->unit].data(), R"(<td  style="text-align:center" >)"))
			return false;
		const float carb=ingr->carb;
		len=snprintf(buf,buflen,"%g",carb);
		if(!writetabnumitem(handle, std::string_view(buf,len)))
			return false;
		const float carbitem= quant*carb;
		 len=snprintf(buf,buflen,"%.1f",carbitem);
		if(!writetabnumitem(handle, std::string_view(buf,len)))
			return false;
		if(writef(handle,rowend,endlen)!=endlen)
			return false;
		totmeal+=carbitem;
		}
	 return doendtable(handle,totmeal,num->gettime()); 
	 }

#include "destruct.h"

extern bool allsavemeals(int handle,uint32_t starttime=0,uint32_t endtime=UINT32_MAX);


extern bool savemeals(FILE* handle,uint32_t starttime,uint32_t endtime);
bool fallsavemeals(FILE *handle,int _unit,uint32_t starttime,uint32_t endtime) {
	if(!(dostarthtml(handle)&&savemeals(handle,starttime,endtime)))
		return false;
	if(fwrite(endhtml.data(),1,endhtml.size(),handle)!=endhtml.size())
		return false;
	return true;
	}


bool allsavemeals(int handle,uint32_t starttime,uint32_t endtime) {
	if(FILE *fp=fdopen(handle,"w")) {
		 bool res=fallsavemeals(fp,0,starttime,endtime) ;
		 fclose(fp);
		 return res;
		 }
	 close(handle);
	return false;
	 }
