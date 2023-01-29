if test $# -ne 1
then
echo "Usage: sh $0 n.n.n  (version number)"
exit
fi
UITPATH=home/jka/src/android/Glucodata/Common/src/main/cpp
hier=`pwd`
cat> $gc/cmdline/version.h<<!
const char version[]="$1";
!
cmake $gc 
make  cmdlinesrc.tar 
tar -xf cmdlinesrc.tar 
cat $gc/CMakeStart.txt > $UITPATH/CMakeLists.txt
grep -v '( *\<g\>'  $gc/CMakeLists.txt|grep -v native|grep -v abbott|grep -v 'TARGE.*\<g\>'|grep -v '^#'|grep -v -i nanovg| sed 's/^\(if.*\)Android\(.*\)$/\1DONTUSE\2/g' >> $UITPATH/CMakeLists.txt
cp $gc/README $UITPATH
cd  $UITPATH/..
mv cpp juggluco-server

tar --xz -cf $hier/juggluco-server$1.src.tar.xz juggluco-server
