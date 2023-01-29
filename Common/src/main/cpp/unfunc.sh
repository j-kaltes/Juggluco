
sed -n -e 's/^\([a-zA-Z 	*]*\)(\*\([A-Za-z]*\))(JNIEnv[ 	]*\*\(,*\)\([^;]*\));$/&\
\1 My\2declarefunc(\4) {\
   logf(\"\2\\n\");\
   return \2callfunc(\4);\
   }\
/p'
