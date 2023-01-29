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


com 
offset len
datanr
dateel: 
filename
namelen
name
data



bool senddata(crypt_t *pass,const int sock,const std::vector<subdata>&data,const std::string_view naar) {
	if(data.size()==0)
		return true;
	const int elnr= data.size();
	const int namelen=naar.size();
	int buflen=sizeof(datel)*elnr+namelen+sizeof(fileonce_t);
	for(auto &el:data) {
		buflen+=el.datalen;	
		}
	std::unique_ptr<senddata_t[],ardeleter<4,senddata_t>> destructptr(new(std::align_val_t(4)) senddata_t[buflen],ardeleter<4,senddata_t>());
	senddata_t *buf=destructptr.get();
	fileonce_t *stru=reinterpret_cast<fileonce_t*>(buf);
	stru->namelen=namelen;
	stru->nr=elnr;
	senddata_t *ptr=reinterpret_cast<sendata_t  *>(stru->gegs+elnr);
	memcpy(ptr,naar.data(),namelen);
	ptr[namelen]='\0';
	ptr+=(namelen+1);
	datel *datar=stru->gegs;
	for(int i=0;i<elnr;i++) {
		const subdata &el=data[i];
		datar[i]={el.offset,el.datalen};
		memcpy(ptr,el.data,el.datalen);
		ptr+=el.datalen;
		}
	stru->totlen=buflen;	
	return sendcommand(pass,sock,buf,buflen);
	}
