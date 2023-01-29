for i in $*
do
 ~/src/bed-3.0.3/utils/bin2byte $i.bin $i $i.h no
 sed -i  's/unsigned char[^{]*\({[^;]*\);.*$/\1/g' $i.h
 done
