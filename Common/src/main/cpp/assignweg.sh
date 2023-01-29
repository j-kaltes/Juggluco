echo '\(' `cat jnisub.cpp |sed -n -e 's/subenv\.\([a-zA-Z][a-zA-Z]*\)\=[^;]*;/\\\\<\1\\\\>\\\\\|/pg'`'niets\)'|sed -e 's/[ 	]*//g'

