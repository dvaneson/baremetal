include ../../Makefile.common

INCPATH = -I ../../simpleio
LIBPATH = -L ../../simpleio -lio

all:	user user2

#----------------------------------------------------------------------------
# A simple user program:
UOBJS	= user.o userlib.o
user:	${UOBJS} user.ld
	$(LD) -T user.ld -o user ${UOBJS} ${LIBPATH} --print-map > user.map
	strip user

user.o:	user.c
	$(CC) ${CCOPTS} ${INCPATH} -o user.o -c user.c

U2OBJS	= user2.o userlib.o
user2:	${U2OBJS} user2.ld
	$(LD) -T user2.ld -o user2 ${U2OBJS} ${LIBPATH} --print-map > user.map
	strip user2

user2.o:	user2.c
	$(CC) ${CCOPTS} ${INCPATH} -o user2.o -c user2.c

userlib.o: userlib.s
	$(CC) -Wa,-alsm=userlib.lst -c -o userlib.o userlib.s

#----------------------------------------------------------------------------
# tidy up after ourselves ...
clean:
	-rm -r user *.o *.lst *.map

#----------------------------------------------------------------------------
