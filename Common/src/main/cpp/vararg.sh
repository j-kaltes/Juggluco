sed -n -e 's/^[ 	]*void[ 	]*(\*\(CallNonvirtual\|CallStatic\|Call\)\([a-zA-Z]*\)Method\>.*$//g;s/^[ 	]*\([a-zA-Z]*\)[ 	]*(\*CallNonvirtual\([a-zA-Z]*\)Method\>.*$/declmethnv(\1,\2,CallNonvirtual\)/gp;s/^[ 	]*\([a-zA-Z]*\)[ 	]*(\*CallStatic\([a-zA-Z]*\)Method\>.*$/declmeths(\1,\2,CallStatic)/gp;s/^[ 	]*\([a-zA-Z]*\)[ 	]*(\*Call\([a-zA-Z]*\)Method\>.*$/declmeth(\1,\2,Call)/gp'


