# build cllsd library
#

# configurable variables
CUTIL_ROOT?=../cutil/src
PREFIX?=/usr/local

# define vars
SHELL=/bin/sh
SRC=base64.c llsd_binary.c llsd.c llsd_const.c llsd_util.c llsd_xml.c 
HDR=base64.h llsd_binary.h llsd.h llsd_const.h llsd_util.h llsd_xml.h
OBJ=$(SRC:.c=.o)
OUT=libcllsd.a
CFLAGS=-O0 -gstabs+ -I$(CUTIL_ROOT)/include

# install utility
INSTALL=/usr/bin/install
INSTALL_PROGRAM=$(INSTALL)
INSTALL_DATA=$(INSTALL) -m 644

all: $(OUT)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT): $(OBJ)
	ar rcs $(OUT) $(OBJ)
	ln -s . cllsd
	ln -s . include
	ln -s . lib

install: all
	mkdir -p $(PREFIX)/include/cllsd
	mkdir -p $(PREFIX)/lib
	$(INSTALL_PROGRAM) --target-directory $(PREFIX)/include/cllsd/ *.h
	$(INSTALL_PROGRAM) --target-directory $(PREFIX)/lib/ *.a

uninstall:
	rm -rf $(PREFIX)/include/cllsd
	rm -rf $(PREFIX)/lib/$(OUT)

test: all

clean:
	rm -rf cllsd
	rm -rf include
	rm -rf lib
	rm -rf $(OBJ)
	rm -rf $(OUT)

.PHONY: all install uninstall clean test

