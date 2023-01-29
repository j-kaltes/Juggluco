sed -n -e 's/^[ 	]*return[ 	]*\([a-zA-Z]*\)callfunc.*$/\1=(*env)->\1;\nenv2.\1=My\1;\n/gp'
