cat > consts.mc <<!
/** This file is automatically generated from nums/const.java in Juggluco. 
*** Making changes in this file without accompanying changes in Juggluco, 
*** will disorganize the communication between Kerfstok and Juggluco.
*/
!

#sed -n 's/[^\/].*public static final int maxstorage=/(:background) const maxstorage=/gp;s/^[^\/].*public static final int\(.*\)$/const\1/gp' < consts.java> consts.mc
sed -n 's/[^\/].*public static final int maxstorage=/const maxstorage=/gp;s/^[^\/].*public static final int\(.*\)$/const\1/gp' < consts.java>> consts.mc
