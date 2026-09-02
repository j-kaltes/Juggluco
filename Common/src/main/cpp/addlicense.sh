#!/bin/bash
if test x$BASH_VERSINFO = x
then
echo $0 should be used as bash $0
exit 1
fi
str='                                                                                   '
datestr=`env LC_NAME=us_EN.UTF-8 date +"%a %b %d %T %Z %Y"`
strlen=${#str}
datelen=${#datestr}
#strleft=$(($strlen-$datelen))
half=6
newdate='/*'"${str:0:$half}$datestr${str:$half+$datelen:$strlen}"'*/'

for inname in $*
do
tmpname=`mktemp  "$inname-tmpXXXXXX"`

mv "$inname" "$tmpname"

(cat<<"!"
/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2(+), Libre 3(+), Dexcom G7/ONE+,        */
/*      Sibionics GS1Sb and GS3, Accu-Chek SmartGuide, CareSens Air and              */
/*      Aidex X sensors.                                                             */
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
!
echo "$newdate"
echo 
cat "$tmpname") > "$inname"
rm "$tmpname"
done

