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
/*      Fri Jan 27 12:38:18 CET 2023                                                 */


#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <string_view>
#include <string>
#include <iostream>
using namespace std;

template <typename T>
bool mkdirs(T &&name,const  mode_t mode=S_IRWXU) {
	int len=name.size();
	char str[len+1]; 
	memcpy(str,name.data(),len);
	str[len]='\0';
	bool success=true;
	for(char *it=str+1;(it=strchr(it,'/'));) {
		*it='\0';	
		int er=mkdir(str,mode);
		int ern=errno;
		success=(er==0||ern==EEXIST)&&success;
		*it++='/';
		}
	int er=mkdir(str,mode);
	int ern=errno;
	success=(er==0||ern==EEXIST)&&success;
	return success;
	}

template <typename T>
std::string_view striplastname(T &&name) {
	auto iter=end(name),start=begin(name);
	iter--;

	while(iter>start&&*iter=='/')
		iter--;

	while(iter>start&&*iter!='/')
		iter--;
	return std::string_view(start,iter-start);
	}

/*
int main(int argc,char**argv) {

string_view path(argv[1]);
auto test=striplastname(path);
cout<<test<<endl;
if(!mkdirs(test,0777))
	perror("mkdir");
}

*/
