# -Zomf
CFLAGS=-g -march=pentium -mcpu=pentium3
#CFLAGS=-g -Ie:/rd/mysql/mysql-4.1.7/include -Ie:/rd/mysql/mysql-4.1.7/mysys
LDFLAGS=-s -Zmap -Zhigh-mem -Zomf -Zexe 
LN_CP_F=cp.exe 
RANLIB=echo 
AR=emxomfar 
AR=ar 

.c.obj:
	gcc $(CFLAGS) -Zomf -o $*.obj -c $*.c

#libpthread.lib
all: libpthread.a 

libpthread.a: my_os2cond.o my_os2mutex.o my_os2thread.o my_os2tls.o
	$(AR) cru libpthread.a my_os2cond.o my_os2mutex.o my_os2thread.o my_os2tls.o
	emxomf -o libpthread.lib libpthread.a


libpthread.lib: my_os2cond.obj my_os2mutex.obj my_os2thread.obj my_os2tls.obj
	emxomfar cru libpthread.lib my_os2cond.obj my_os2mutex.obj my_os2thread.obj my_os2tls.obj
