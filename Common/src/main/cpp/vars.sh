
infile=$1
for((num=1;$num<27;num++))
do
uitfile=/tmp/omfunc$num

sed -e "s/declarefun\(.*\)\([c0-9][(,]\)[ 	]*\([a-zA-Z_][a-zA-Z_]*\)\([ 	]*\)\([a-zA-Z_]*\)[ 	]*\([*]*\)\([,)]\)\([^0-9][^0-9]*\)*$/declarefun\1\2\3\4\5 \6 \3$num\7\8/g"  -e "s/callfun\(.*\)\([c0-9][(,]\)[	 ]*\([a-zA-Z_][a-zA-Z_]*\)\([ 	]*\)\([a-zA-Z_]*\)[ 	]*\([*]*\)\([,)]\)\([^0-9][^0-9]*\)*$/callfun\1\2\3$num\7\8/g"  < $infile > $uitfile

infile=$uitfile
done

sed -e 's/declarefunc()/(JNIEnv*env)/g;s/callfunc()/(env)/g;s/declarefunc(\(..*\))/(JNIEnv*env,\1)/g;s/callfunc(\(..*\))/(env,\1)/g' < $infile > $2


