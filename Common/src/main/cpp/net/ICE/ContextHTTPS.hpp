/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2, Libre 3, Dexcom G7/ONE+,              */
/*      Sibionics GS1Sb and Accu-Chek SmartGuide sensors.                            */
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
/*      Fri Nov 21 11:08:14 CET 2025                                                 */
#pragma once 
#include <vector>
#include <string_view>
#include <span>
#include <openssl/ssl.h>

using namespace std::literals;
class ContextHTTPS {
private:
    SSL_CTX* ctx ;
    bool error=false;
static bool initLibrary();
public:

    static ContextHTTPS &getContext() ;
    ContextHTTPS();
    ~ContextHTTPS();
std::pair<std::vector<char>,int>  request(const std::string_view host,int port,const std::string_view path,const std::string_view TYPE,const std::span<const char> input) ;
std::pair<std::vector<char>,int>   getRequest(const std::string_view host,int port,const std::string_view path,const std::span<const char> input={})  {
    return  request(host, port,path,"GET"s, input) ;
    }
std::pair<std::vector<char>,int>  putRequest(const std::string_view host,int port,const std::string_view path,const std::span<const char> input={})  {
    return  request(host, port,path,"PUT"s, input) ;
    }
 };
