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
/*      Fri Jan 27 15:20:04 CET 2023                                                 */


#pragma once
#include <math.h>

#include <iostream>
using namespace std;
template <int  n,int e>
constexpr unsigned long long nattus() {
	if constexpr (n<=e)
		return 1;
	else
		return n*nattus<n-1,e>();
	}
template <int n>
constexpr const long double  nat()  {
	return nattus<n,1>();
	}	
template <int n,int e>
constexpr const long double  nat2()  {
	return nattus<n,e>();
	}	

template <int n,int k> static constexpr double  func() {
	return nat2<2*n,n+k>()/(nat<n-k>()*powl(4.0,n));
	}
template<int n,int k> void init(double *weight) {
	if constexpr (k==0)
		weight[n]=func<n,0>();
	else {
		weight[n-k]=weight[n+k]=func<n,k>();
		init<n,k-1>(weight);
		}
	}
template <int n>
class filter {
static constexpr int totlen= (2*n+1);
double weight[totlen];
public:



filter() {
	init<n,n>(weight);
	}
double get(int ind) {
	return weight[ind];
	}
template <typename T>
T smooth(int pos, const T *perc,int idnr) {
        int half=n;
        float uit=0.0f;
	double mis=1.0;
        for(int i=0;i<totlen;i++) {
		auto val=perc[(idnr+pos-half+i)%idnr];
		if(val)
			uit+=weight[i]*val;
		else 
			mis-=weight[i];
                }
	
        return roundf(uit/mis);
        }
template <typename T>
T   *smoothar(const T *perc,int idnr) {
                T *ar=new T[idnr];
                for(int i=0;i<idnr;i++) {
			ar[i]=perc[i]?smooth(i,perc,idnr):0;
                        }
                return ar;
                }


void print() {
	for(double el:weight) 
		cout<<el<<' ';
	cout<<endl;
	}
void printh() {
	for(int i=n;i<totlen;i++)
		cout<<weight[i]<<' ';
	cout<<endl;
	}

};

/*
static filter<20> filt;
extern double func(int ind) ;
double func(int ind) {
	return filt.get(ind);
	}
int main() {
filter<2>().printh();

filter<3>().printh();
filter<4>().printh();
filter<5>().printh();
filter<6>().printh();
filter<7>().printh();
filter<8>().printh();
//constexpr double get=nat<5>();
//cout<<get<<endl;
//one.print();
//return func(5)*100;
}
*/	
