sed -e 's/JNIEXPORT/extern "C"\
&/g' -e 's/^[^A-Za-z]*\(.JNIEnv.*\);$/\1 {};/g'

