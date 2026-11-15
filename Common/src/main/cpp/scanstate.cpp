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


#include "scanstate.hpp"
#include <string.h>
#include <cstdlib>
#include <string>
#include "timestr.h"
data_t *scanstate::fromfile(const char *filename) {
    Open file(filename,O_RDONLY);
    if(file<=0)
        return nullptr;
    struct stat st;
    if(fstat(file,&st)==-1)
        return nullptr;
    data_t *data= alloc(st.st_size); 
    if(read(file,data->buf,st.st_size)!=st.st_size) {
        return nullptr;
        }
    return data;    
    }

bool scanstate::makelink(string_view filename) {
    if(filename.length()<=0)
        return false;
    decltype(auto) pos=filename.find_last_of('/');
    if(pos==filename.npos)
        return false;
    pos++;
    constexpr const char linkend[]="state.lnk";
    int buflen=pos+sizeof(linkend);
    char buf[buflen];
    memcpy(buf,&filename[0],pos);
    memcpy(buf+pos,linkend,sizeof(linkend));
#ifdef LIBRE2_EU_INDEPENDENT
    // Keep the previous link readable until the replacement is complete.
    std::string temporary=std::string(buf)+".XXXXXX";
    const int fd=mkstemp(temporary.data());
    if(fd<0)return false;
    const auto length=filename.length()-pos;
    const bool written=write(fd,filename.data()+pos,length)==static_cast<ssize_t>(length);
    const bool closed=close(fd)==0;
    if(written&&closed&&rename(temporary.c_str(),buf)==0)return true;
    unlink(temporary.c_str());
    return false;
#else
    return writeall(buf,filename.data()+pos,filename.length()-pos);
#endif
}
string_view scanstate::makefilename(const string_view sbasedir,const time_t tim) { //Has to be deleted
    constexpr const char start[]="/state";
    int baselen= sbasedir.length();
    if(sbasedir[baselen-1]=='/')
        baselen--;
    const string_view::size_type buflen=baselen+timestrlen+sizeof(start);
    char *filename=new char[buflen];  //*******
    constexpr int startlen=sizeof(start)-1;
    memcpy(filename,&sbasedir[0],baselen);
    memcpy(filename+baselen,start,startlen);
    timestr(filename+baselen+startlen,tim);
    LOGGER("makefilename()=%s\n",filename);
    return {filename,buflen-1};
    }
scanstate::scanstate(string_view prev,SavedApart) : multimmap(4,4*4096) {
    constexpr int maxpath=512;
    char buf[maxpath];
    int dirlen=prev.length();
    memcpy(buf,prev.data(),dirlen);
    if(buf[dirlen-1]!='/')
        buf[dirlen++]='/';
    strcpy(buf+dirlen,"mess.dat");setpos(MESS, fromfile(buf));
    strcpy(buf+dirlen,"composite.dat"); setpos(COMP,fromfile(buf));
    strcpy(buf+dirlen,"attenuation.dat"); setpos(ATTE,fromfile(buf));
    }

void scanstate::removefile() {
    if(unlink(filename.data())!=0) {
        lerror(        filename.data());
        };
    };
