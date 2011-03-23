# $Id$

SUBDIRS = elfmark lnxstamp mkmnttab # trace

all: ${SUBDIRS}

elfmark::
	@( echo "$@ ===> elfmark"; cd elfmark; ${MAKE} )

lnxstamp::
	@( echo "$@ ===> lnxstamp"; cd lnxstamp; ${MAKE} )

mkmnttab::
	@true # nothing

#trace::
#	@( echo "$@ ===> trace"; cd trace; ${MAKE} )

install clean::
	@for i in ${SUBDIRS}; do \
	    ( echo "$@ ===> $$i" ; cd $$i ; ${MAKE} $@ ); \
	done
