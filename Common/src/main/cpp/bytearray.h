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
/*      Fri Jan 27 12:35:09 CET 2023                                                 */



struct ByteArray {
jsize len;
jbyte  *bytes;
ByteArray(jsize len): len(len),bytes(new jbyte[len]()){
LOGGER("ByteArray(%d)\n",len);
}
ByteArray(ByteArray &&in): len(in.len),bytes(in.bytes){
	in.bytes=nullptr;
	in.len=0;
		LOGGER("ByteArray(&&%d)\n",len);
	}
ByteArray(const ByteArray &in): len(in.len),bytes(new jbyte[len]){
	memcpy(bytes,in.data(),len);
	LOGGER("ByteArray(&&%d)\n",len);
	}
template <typename T,  std::size_t  N> ByteArray(const std::array<T,N> &ar): ByteArray(N*sizeof(T)) {
	memcpy(bytes,ar.data(),len);
	}
/*ByteArray(std::array<T,N> &&ar): bytes(reinterpret_cast<jbyte *>(ar.data())), len(N) {
	}
	*/
static ByteArray *readfile(const char name[]) ;
jsize length() const {return len;}
~ByteArray() {
	delete[] bytes;
	}
jbyte *data() {return bytes;}
const jbyte *data() const {return bytes;}
operator jbyteArray() {return (jbyteArray)this;}
operator jbyte*() {return bytes;}
};
ByteArray *ByteArray::readfile(const char name[]) {
	struct stat st;
	if(stat(name,&st)==-1) {
		return nullptr;
		}
	ByteArray *ar= new ByteArray(st.st_size);
	if(::readfile(name,ar->bytes,ar->len) )
		return ar;
	delete ar;
	return nullptr;
	}
