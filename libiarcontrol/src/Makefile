
CC = gcc
CFLAGS = -O2 -Wall
#CFLAGS +=  -DVERBOSE_MODE
#CFLAGS +=  -DDEBUG_MODE
CFLAGS += -I../include 

VERSION := $(shell cat ../VERSION )
MAINVERSION:=$(shell cat ../VERSION |sed 's/^\([0-9]*\)\..*/\1/')
MAYOR := $(shell awk -F"." '{print $$1}' ../VERSION)
HEADLIST := $(shell cd ../include; ls -1 *.h)
LIBS=libiarcontrol.a
LIBSSO=libiarcontrol.so
TARGET=/usr
LIBOBJS=my_stdio.o \
	iar_bus.o iar_bus_client.o \
	socket_client.o socket_srv.o \
	ib_fd.o \
	my_math.o my_stdlib.o \
	flags.o \
	locks.o parse_line_conf.o parse_configuration.o parse_conf_array.o \

#DUMMYROOT=

all: .depend ${LIBS} ${LIBSSO}.${VERSION}

clean:
	rm -f *.o
	rm -f $(LIBS)
	rm -f .depend
	rm -f core
	rm -f ${LIBSSO}.${VERSION} ${LIBSSO} ${LIBSSO}.${MAINVERSION}

libiarcontrol.a: ${LIBOBJS} 
	ar rcs $@ $^
	ranlib $@

${LIBSSO}.${VERSION}: ${LIBOBJS}
	gcc -shared -Wl,-soname,${LIBSSO}.${MAYOR} \
	-o ${LIBSSO}.${VERSION} $^ -lc -lm
	ln -sf ${LIBSSO}.${VERSION} ${LIBSSO}
	ln -sf ${LIBSSO}.${VERSION} ${LIBSSO}.${MAINVERSION}


shared: ${LIBSSO}.${VERSION}

.c.o:
	gcc $(CFLAGS) -fPIC -c $< -o $@

install: all installdev
	test -d $(DESTDIR)/$(TARGET)/lib||mkdir -p $(DESTDIR)/$(TARGET)/lib
	install -p ${LIBSSO}.${VERSION} $(DESTDIR)/${TARGET}/lib
	(ldconfig; cd $(DESTDIR)/${TARGET}/lib; rm -f ${LIBSSO}; \
	 ln -sf ${LIBSSO}.${VERSION} ${LIBSSO}; \
	 ln -sf ${LIBSSO}.${VERSION} ${LIBSSO}.${MAINVERSION})

installdev: all
	test -d $(DESTDIR)/$(TARGET)/lib||mkdir -p $(DESTDIR)/$(TARGET)/lib
	test -d $(DESTDIR)${TARGET}/include||mkdir -p $(DESTDIR)${TARGET}/include
	install -p -m 644 libiarcontrol.a $(DESTDIR)/${TARGET}/lib
	install -p -m 644 ../include/* $(DESTDIR)${TARGET}/include

uninstall:
	(cd $(DESTDIR)/${TARGET}/lib; rm -f ${LIBSSO}.*;)
	(cd $(DESTDIR)${TARGET}/include; rm -f ${HEADLIST})


cleanb: clean
	rm -f *~

dep:
	rm -f .depend
	make .depend

depend .depend:
	$(CC) $(CFLAGS) -M *.c > $@

deb: 
	sed 's/^Version:.*/Version: $(VERSION)/' DEBIAN/control > DEBIAN/control.v
	test -s DEBIAN/control.v && mv -f DEBIAN/control.v DEBIAN/control
	make_deb ~/Projects/my_pool

ifeq (.depend,$(wildcard .depend))
include .depend
endif


