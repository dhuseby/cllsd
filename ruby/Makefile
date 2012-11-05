# build ruby llsd module
#

# configurable variables
CUTIL_ROOT?=../cutil/src
EXTRA_LIBS_ROOT?=/usr/local

SHELL=/bin/sh
SRC=llsd.c
OBJ=$(SRC:.c=.o)
GCDA=$(SRC:.c=.gcda)
GCNO=$(SRC:.c=.gcno)
GCOV=$(SRC:.c=.c.gcov)
OUT=llsd.so
LIBS=-lcllsd -lcutil -lexpat -lm
CLLSD_ROOT=../src
INTERFACE=$(CLLSD_ROOT)/llsd.i
CFLAGS=-O0 -gstabs+ -I$(CLLSD_ROOT)/include -I$(CUTIL_ROOT)/include -I/usr/lib/ruby/1.8/i686-linux
LDFLAGS=-shared -L$(CLLSD_ROOT)/lib -L$(CUTIL_ROOT)/lib

all: $(OUT)

test:

testnr:

coverage:

# generate llsd.c from the llsd.i interface file
$(SRC): $(INTERFACE)
	swig -o $@ -ruby $^

$(OBJ): $(SRC)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

install:

uninstall:

report:

clean:
	rm -rf $(OBJ)
	rm -rf $(OUT)
	rm -rf $(SRC)

.PHONY: all install uninstall clean test coverage report

