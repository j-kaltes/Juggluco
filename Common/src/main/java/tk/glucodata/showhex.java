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
/*      Fri Jan 27 15:31:05 CET 2023                                                 */


package tk.glucodata;
final class showhex {

private static char  bchar(byte b) {
	return (char)(b>=0xA?(b-0xA+(byte)'A'):b+(byte)'0');
	}
public static char[] hexstr(final byte[] bytes,int start,int len) {
	char[] chars=new char[len*2];	
	for(int i=0;i<len;i++) {
		byte by=bytes[start+i];
		chars[i*2]=bchar((byte)((by>>4)&(byte)0xF));
		chars[i*2+1]=bchar((byte)(by&(byte)0xF));
		}
	return chars;
	}
	/*
public static void main(String args[]) {
	byte[] by={(byte)0x98,(byte)0xab,(byte)0xcd,(byte)0xef};	
	char[] ch=hexstr( by,0,4);
	System.out.println(ch);
	}
	*/
public static String showbytes(byte[] key) {
	return new String(showhex.hexstr(key, 0, key.length));
	}
}
