/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2, Libre 3, Dexcom G7/ONE+ and           */
/*      Sibionics GS1Sb sensors.                                                     */
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
/*      Sat Jul 05 14:36:26 CEST 2025                                                */


#pragma once
#include <algorithm>
#include <numeric>
#include <string_view>
#include <span>
#include <math.h>
#ifndef LOGGERN
#include "logs.hpp"
#endif
constexpr inline auto square(const auto x) { return x*x; }
template <typename Cont> auto getNumeratorA(const Cont & w,const Cont & x, const Cont & y,const int nr) {
    const int n1=nr-1;
    long double res{};
    for(int i=0;i<n1;++i)  {
        const auto xi=x[i];
        const auto yi=y[i];
        long double term{};
        for(int j=i+1;j<nr;++j) {
            term+=w[j]*(xi-x[j])*(yi-y[j]);
            }
        res+=w[i]*term;
        }
    return res;
    }
template <typename Cont> auto getDenominatorA(const Cont & w,const Cont & x, const int nr) {
    const int n1=nr-1;
    long double res{};
    for(int i=0;i<n1;++i)  {
        long double term{};
        const auto xi=x[i];
        for(int j=i+1;j<nr;++j) {
            term+=w[j]*square(xi-x[j]);
            }
        res+=w[i]*term;
        }
     return res;
    }
template <typename Cont> 
auto    getA(Cont & w,Cont & x, Cont & y,int nr) {
        return getNumeratorA(w,x,y,nr)/getDenominatorA(w,x,nr);
        }

template <typename Cont> auto getNumeratorB(const Cont & w,const Cont & x, const Cont & y,const int nr) {
    const int n1=nr-1;
    long double res{};
    for(int i=0;i<n1;++i)  {
        const auto xi=x[i];
        const auto yi=y[i];
        long double term{};
        for(int j=i+1;j<nr;++j) {
            term+=w[j]*(xi-x[j])*(xi*y[j]-x[j]*yi);
            }
         res+=w[i]*term;
         }
     return res;
     }

template <typename Cont> auto getDenominatorB(const Cont & w,const Cont & x, const int nr) {
    const int n1=nr-1;
    long double res{};
    for(int i=0;i<n1;++i)  {
        long double term{};
        const auto xi=x[i];
        for(int j=i+1;j<nr;++j) {
            term+=w[j]*square(xi-x[j]);
            }
        res+=w[i]*term;
        }
     return res;
    }
template <typename Cont> 
auto    getB(Cont & w,Cont & x, Cont & y,int nr) {
        return getNumeratorB(w,x,y,nr)/getDenominatorB(w,x,nr);
        }
 
/*constexpr double moderateA(double a,int n,double mod) {
    return (n*a + mod)/(n + mod);
    } */
constexpr double moderateA(double a,double n,double mod) {
    return a - (a-1)* exp(-(n/mod));
    }
    /*
constexpr double moderateB(double b,int n,double mod) {
    return b*n/(n+mod);
    } */
constexpr double moderateB(double b,double n,double mod) {
     return b*(1.0 - exp(-n/mod));
    }
template <typename Cont>
auto    distance(Cont & w,Cont & x, Cont & y,int nr) {
        long double tot{};
        for(int i=0;i<nr;++i) { 
                tot+=w[i]*(y[i]-x[i]);
                }
        return tot; 
        }
template <typename Cont>
inline static void printvector(const char *other,std::string_view name,Cont &x) {
#ifndef NOLOG
    constexpr const int maxbuf=1024;
    int index=0;
    char buf[maxbuf];
    index+=snprintf(buf+index,maxbuf-index,"%s %s= ",other,name.data());
    for(auto el:x) {
        index+=snprintf(buf+index,maxbuf-index,"%.2f ",el);
        }
    LOGGERN(buf,index);
#endif
    }

template  <typename DT>
inline auto valid(DT &el) {
    return el.valid();
    }

template  <typename DT>
inline auto getmgdL(DT &el) {
    return el.getmgdL();
    }

inline auto valid(double el) {
    return true;
    }

inline auto getmgdL(double el) {
    return el;
    }
template <typename Cont>
inline std::pair<long double,int> mean_mgdL(Cont &data) {
    double long sum{};
    int count{};
    for(const auto &el:data) {
        if(valid(el)) {
            ++count;
            sum+=getmgdL(el);
            }
         
        }
     return {sum/count,count};
    }

template <typename Sens,typename DT>
inline std::pair<long double,int> mean_mgdL(Sens *sens,int start,int end) {
    double long sum{};
    int count{};
    for(int i=start;i<end;++i) {
        auto &el=*getdata<DT>(sens,i); 
        if(valid(el)) {
            ++count;
            sum+=getmgdL(el);
            }
         
        }
     return {sum/count,count};
    }
template <typename Sens,typename DT>
inline long double variance_mgdL(Sens *sens,long double mean,int countMin1,int start, int end) {
    double long sum{};
    for(int i=start;i<end;++i) {
        auto &el=*getdata<DT>(sens,i); 
        if(valid(el)) {
            sum+=square(getmgdL(el)-mean);
            }
        }
     return sum/countMin1;
    }

template <typename Cont>
inline long double variance_mgdL(long double mean,int countMin1,Cont& data) {
    double long sum{};
    for(const auto &el:data) {
        if(valid(el)) {
            sum+=square(getmgdL(el)-mean);
            }
        }
     return sum/countMin1;
    }
template <typename Cont>
inline long double sd_mgdL(long double mean,int countMin1,Cont &data) {
    return sqrt(variance_mgdL(mean,countMin1,data));
    }

template <typename Sens,typename DT>
inline long double sd_mgdL(Sens *sens,long double mean,int countMin1,int start,int end) {
    return sqrt(variance_mgdL<Sens,DT>(sens,mean,countMin1,start,end));
    }

template <typename Cont>
inline long double sd_mgdL(Cont &data) {
     const auto [mean,countMin1]=mean_mgdL(data);
    return sqrt(variance_mgdL(mean,countMin1,data));
    }


template <typename Sens,typename DT>
inline long double sd_mgdL(Sens *sens) {
    int start=getfirstpos<DT>(sens);
    int end=getlastpos<DT>(sens);
     const auto [mean,countMin1]=mean_mgdL<Sens,DT>(sens,start,end);
    return sqrt(variance_mgdL<Sens,DT>(sens,mean,countMin1,start,end));
    }

