TARGET=$$HOME


CFLAGS = -O2 -Wall
# host libertad is my debuger platform 
ifeq ($(GCAMPDAHOST),libertad)
#CC = gcc
CC = colorgcc
#CFLAGS += -std=c99
CFLAGS += -DDEBUG_MODE
CFLAGS += -DVERBOSE_MODE
else
CC = gcc
#CC = colorgcc
#CFLAGS += -std=c99
#CFLAGS += -DDEBUG_MODE
#CFLAGS += -DVERBOSE_MODE
endif
CFLAGS += -I.

### 
ifeq (libiarcontrol,$(wildcard libiarcontrol))
CFLAGS += -Ilibiarcontrol/include
LIBS=libiarcontrol.a
STATIC_LIBS=$(LIBS)
else
LIBS=-liarcontrol
STATIC_LIBS=
endif
###

#LIBOBJS=my_stdio.o flags.o parse_line_conf.o

BASEDIR := $(shell basename `pwd`)
VERSION := $(shell cat VERSION)
DISTRIBUTION=..
TARDIST=${BASEDIR}-${VERSION}.tgz


all: .depend $(STATIC_LIBS) gcampda 

gcampda: version.h main.o rs232.o pakbus.o csi.o cr_fd.o cr_tdf.o \
	gcampda_conf.o gcampda.o my_io.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)
	strip $@

libiarcontrol.a:
	make -C libiarcontrol/src $@
	ln -fs libiarcontrol/src/$@ .

clean:
	rm -f *.o
	rm -f .depend
	rm -f gcampda 
ifeq    (libiarcontrol,$(wildcard libiarcontrol))
	make -C libiarcontrol/src $@
endif
	rm -f libiarcontrol.a

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

cleanb: clean
	rm -f *~

dep:
	rm -f .depend
	make .depend

depend .depend:
	$(CC) $(CFLAGS) -M *.c  > $@

version.h: VERSION
	echo "#define VERSION \"$(shell cat VERSION)\"" >version.h


install: all
	test -d $(DESTDIR)/$(TARGET)/bin||mkdir -p $(DESTDIR)/$(TARGET)/bin
	test -d $(DESTDIR)/etc ||mkdir -p $(DESTDIR)/etc
	install -p gcampda $(DESTDIR)/$(TARGET)/bin/
	install -p gcampda.conf $(DESTDIR)/etc

uninstall:
	rm -f $(DESTDIR)/$(TARGET)/bin/gcampda
	rm -f $(DESTDIR)/etc/gcampda.conf


filelist:
	echo ${BASEDIR}/gcampda.conf     > .filelist
	echo ${BASEDIR}/INSTALL         >> .filelist
	echo ${BASEDIR}/TODO            >> .filelist
	( cd .. ;\
	find ${BASEDIR} -name "Makefile" -print >> ${BASEDIR}/.filelist;\
	find ${BASEDIR} -name "VERSION" -print >> ${BASEDIR}/.filelist;\
	find ${BASEDIR} -name "*.[ch]" -print >> ${BASEDIR}/.filelist;\
	)

tar: filelist
	cd ..; tar cvTf ${BASEDIR}/.filelist - | \
        gzip -v9 > ${BASEDIR}/${DISTRIBUTION}/${TARDIST}



ifeq (.depend,$(wildcard .depend))
include .depend
endif

