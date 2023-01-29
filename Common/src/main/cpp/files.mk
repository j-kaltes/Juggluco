
subtrace.h: trace.h assignweg.grep subtrace.sh
	sh subtrace.sh < trace.h |grep -v -f assignweg.grep> subtrace.h
subtrace.c: jni-line.h subweg.grep vars.sh unfunc.sh subtracec.sh
	grep -v -f subweg.grep<jni-line.h |sh unfunc.sh > jnisub2.h
	sh vars.sh jnisub2.h jnisub3.c
	sh subtracec.sh < jnisub3.c > subtrace.c

jni2.h:jni-line.h unfunc.sh  Makefile weg.grep
	grep -v -f weg.grep<jni-line.h |sh unfunc.sh > jni2.h
jni3.h: jni-line.h unfunc.sh  Makefile
	 sh unfunc.sh <jni-line.h > jni3.h
jni-line.h:jni.h Makefile
	sed -z -e 's/,[ 	]*\n[ 	]*/,/g'  < jni.h> jni-line.h
trace.c:jni2.h  vars.sh
	sh vars.sh jni2.h trace.c
vararg.h:jni-line.h	vararg.sh 
	sh vararg.sh <jni-line.h > vararg.h

trace.h: jni3.h
	sh roep.sh < jni3.h>trace.h

tracedec.h:jni-line.h
	sh header.sh



weg.grep:done.sh abbott.cpp jnitrace.c jnitrace2.cpp
	sh done.sh > weg.grep
subweg.grep:subdone.sh jnisub.cpp
	sh subdone.sh > subweg.grep
assignweg.grep: jnisub.cpp assignweg.sh
	sh assignweg.sh > assignweg.grep


backup:
	tar -cf backup.tar subtrace.h trace.c vararg.h trace.h tracedec.h weg.grep
