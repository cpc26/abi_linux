# $Id$

PROG=	elfmark
OBJS=	elfmark.o

BINDIR=	/usr/local/sbin


all: ${PROG}

clean::
	rm -f ${PROG} ${OBJS}

install: ${PROG}
	install ${PROG}	${BINDIR}


${PROG}: ${OBJS}
	${CC} -o ${PROG} ${OBJS}
