# build cllsd test executable
#

# configurable variables
CUTIL_ROOT?=../cutil/src

SHELL=/bin/sh
SRC=test_all.c test_base16.c test_base64.c test_base85.c test_binary.c test_xml.c
OBJ=$(SRC:.c=.o)
GCDA=$(SRC:.c=.gcda)
GCNO=$(SRC:.c=.gcno)
GCOV=$(SRC:.c=.c.gcov)
OUT=test_all
LIBS=-lcllsd -lcutil -lcunit -lexpat -lm
CLLSD_ROOT=../src
CFLAGS=-O0 -gstabs+ -I$(CLLSD_ROOT)/include -I$(CUTIL_ROOT)/include
LDFLAGS=-gstabs+ -L$(CLLSD_ROOT)/lib -L$(CUTIL_ROOT)/lib
# set LDFLAGS to something gcov can use
coverage: LDFLAGS=-fprofile-arcs -O0 -gstabs+ -L$(CLLSD_ROOT)/lib -L$(CUTIL_ROOT)/lib -L$(EXTRA_LIBS_ROOT)/lib

all:

test: $(OUT)
	./test_all

coverage: $(OUT)
	./test_all

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
	rm -rf $(GCDA)
	rm -rf $(GCNO)
	rm -rf $(GCOV)

.PHONY: all install uninstall clean test coverage report

