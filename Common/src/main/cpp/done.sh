echo '\(' `cat abbott.cpp jnitrace.c jnitrace2.cpp | sed  -n -e 's/^.*My\([A-Za-z]*\)(.*{.*$/\\\\<\1\\\\>\\\\\|/pg'`'\.\.\. \)'|sed -e 's/[ 	]*//g'
