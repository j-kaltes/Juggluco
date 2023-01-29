cat>tracedec.h <<"!"

#ifdef __cplusplus
extern "C" {
#endif
!
sed -n -e 's/^\([^(]*([ 	]*[	 ]*[^)]*)[ 	]*([ 	]*\)JNIEnv\(.*\)$/extern \1const struct JNINativeInterface *\2/gp' < jni-line.h >>tracedec.h
cat>>tracedec.h <<"!"
#ifdef __cplusplus
}
#endif
!
