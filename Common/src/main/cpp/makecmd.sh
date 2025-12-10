if test $# -ne 1
then
echo "Usage: sh $0 n.n.n  (version number)"
exit
fi
UITPATH=home/jka/src/android/Glucodata/Common/src/main/cpp
hier=`pwd`
cat> $gc/cmdline/version.h<<!
#ifndef APPVERSION 
#define APPVERSION "$1"
#endif
!
cmake $gc 
make  cmdlinesrc.tar 
tar -xf cmdlinesrc.tar 
grep -v '( *\<g\>'  $gc/CMakeLists.txt|grep -v native|grep -v abbott|grep -v 'TARGE.*\<g\>'|grep -v '^#'|grep -v -i javacurve| sed 's/^\(if.*\)Android\(.*\)$/\1DONTUSE\2/g' >> $UITPATH/CMakeLists.txt
mkdir $UITPATH/libjuice
cp -a  $gc/libjuice/cmake $UITPATH/libjuice
cp $gc/libjuice/CMakeLists.txt $UITPATH/libjuice
cp $gc/README $UITPATH
cd  $UITPATH/..
mv cpp juggluco-server

tar --xz -cf $hier/juggluco-server$1.src.tar.xz juggluco-server
