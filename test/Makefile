# build cllsd test executable
#
SHELL=/bin/sh
SRC=test_all.c test_binary.c test_xml.c
OBJ=$(SRC:.c=.o)
OUT=test_all
LIBS=-lcllsd -lcutil -lcunit -lexpat
CLLSD_ROOT=../src
CUTIL_ROOT=/usr/local
CFLAGS=-O3 -gstabs+ -I$(CLLSD_ROOT)/include -I$(CUTIL_ROOT)/include
LDFLAGS=-gstabs+ -L$(CLLSD_ROOT)/lib -L$(CUTIL_ROOT)/lib

all:

test: $(OUT)
	./test_all

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

install:

uninstall:

clean:
	rm -rf $(OBJ)
	rm -rf $(OUT)

.PHONY: all install uninstall clean test

