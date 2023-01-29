echo '\(' `cat jnisub.cpp | sed  -n -e 's/^.*\<sub\([A-Za-z]*\)([ 	]*[a-zA-Z].*).*{.*$/\\\\<\1\\\\>\\\\\|/pg'`'niets\)'|sed -e 's/[ 	]*//g'
