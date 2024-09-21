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
/*      Wed May 01 10:52:38 CEST 2024                                                 */



#pragma once
#include <unordered_map>
#include "scanstate.hpp"
struct Method {
string name;
string pat;
void *func;
};


struct jclasser
{
std::vector<Method> funcs;

static std::unordered_map<string,jclasser*> classid; 
jclasser(const char name[], std::vector<Method> &&funcs): funcs(std::move(funcs)) {
	classid.emplace( std::make_pair(name, this));
	}
};
struct algobj {
public:
 jclasser *cl;
void *ptr=nullptr;
algobj(jclasser *cl):cl(cl) {}
operator jobject() {
        return (jobject)this;
        }
	/*
operator intptr_t() {
	return reinterpret_cast<intptr_t>(ptr);
	}
	*/
intptr_t toint() {
	return reinterpret_cast<intptr_t>(ptr);
	}
};
extern jclasser out;
struct outobj :public algobj{
outobj(): algobj(&out) {}
// jclasser *algobj::cl=&out;
};
struct jnidata_t {
	const struct JNINativeInterface* functions;
	scanstate *map;
	};

class PatchEvent {
private:
    const int id;
    const int errorCode;
public:
    PatchEvent(int id, int code) :id(id),errorCode(code) { }

   int getId()const { return id; }

   int getErrorCode() const { return errorCode; }
};
extern JNIEnv *subenv;

extern bool javaAttached;
