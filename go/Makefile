# build go llsd module
#

# configurable variables
CUTIL_ROOT?=../cutil/src
EXTRA_LIBS_ROOT?=/usr/local

SHELL=/bin/sh
SRC=llsd.c
GOSRC=llsd_gc.c
OBJ=$(SRC:.c=.o) 
GOOBJ=$(GOSRC:.c=.o)
GCDA=$(SRC:.c=.gcda)
GCNO=$(SRC:.c=.gcno)
GCOV=$(SRC:.c=.c.gcov)
OUT=_llsd.so
LIBS=-lcllsd -lcutil -lexpat -lm
CLLSD_ROOT=../src
INTERFACE=$(CLLSD_ROOT)/llsd.i
CFLAGS=-O0 -gstabs+ -fPIC -I$(CLLSD_ROOT)/include -I$(CUTIL_ROOT)/include -I$(GOROOT)/pkg/$(GOOS)_$(GOARCH)
LDFLAGS=-shared -L$(CLLSD_ROOT)/lib -L$(CUTIL_ROOT)/lib

all: $(OUT)

test:

testnr:

coverage:

# generate llsd.c from the llsd.i interface file
$(SRC): $(INTERFACE)
	swig -package llsd -intgo-type-size 32 -long-type-size 32 -o $@ -go $^

$(OBJ): $(SRC)

$(SRC):
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

$(GOOBJ): $(GOSRC)

$(GOSRC):
	$(GO6C) $<

install:

uninstall:

report:

clean:
	rm -rf $(OBJ)
	rm -rf $(OUT)
	rm -rf $(SRC)
	rm -rf llsd.go*

.PHONY: all install uninstall clean test coverage report

