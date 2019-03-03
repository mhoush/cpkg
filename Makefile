# cpkg/pkgutils Makefile

VERSION = 6.0.0

CFLAGS = -O2 -Wall -DVERSION=\"$(VERSION)\" \
         -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS += -DDEBUG -g

LIBS = -larchive

all: cpkg pkginfo

cpkg:
	$(CC) $(CFLAGS) $(LIBS) -o cpkg *.c

pkginfo:
	ln -s cpkg pkginfo

clean:
	rm -f cpkg pkginfo
