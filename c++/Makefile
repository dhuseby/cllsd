# build C++ llsd wrapper
#

# configurable variables
CUTIL_ROOT?=../cutil/src
PREFIX?=/usr/local
COVERAGE?=../coverage

SHELL=/bin/sh
SRC=llsd++.cpp
HDR=llsd++.h
OBJ=$(SRC:.c=.o)
GCDA=$(SRC:.c=.gcda)
GCNO=$(SRC:.c=.gcno)
GCOV=$(SRC:.c=.c.gcov)
OUT=lib$(NAME)++.a
CLLSD_ROOT=../src
CFLAGS=-O0 -gstabs+ -I$(CLLSD_ROOT)/include -I$(CUTIL_ROOT)/include
coverage: CFLAGS=-fprofile-arcs -ftest-coverage -O0 -gstabs+ -DUNIT_TESTING -I$(CUTIL_ROOT)/include
test: CFLAGS=-O0 -gstabs+ -DUNIT_TESTING -I$(CUTIL_ROOT)/include


# install utility
INSTALL=/usr/bin/install
INSTALL_PROGRAM=$(INSTALL)
INSTALL_DATA=$(INSTALL) -m 644

all: $(OUT)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT): $(OBJ)
	ar rcs $(OUT) $(OBJ)
	rm -rf $(NAME)
	rm -rf include
	rm -rf lib
	ln -s . $(NAME)
	ln -s . include
	ln -s . lib

install: all
	mkdir -p $(PREFIX)/include/$(NAME)
	mkdir -p $(PREFIX)/lib
	$(INSTALL_PROGRAM) --target-directory $(PREFIX)/include/$(NAME)/ *.h
	$(INSTALL_PROGRAM) --target-directory $(PREFIX)/lib/ *.a

uninstall:
	rm -rf $(PREFIX)/include/$(NAME)
	rm -rf $(PREFIX)/lib/$(OUT)

test: all

coverage: all

report:
	lcov --base-directory . --directory . -c -o cutil.info
	lcov --remove cutil.info "/usr*" -o cutil.info
	rm -rf $(COVERAGE)
	genhtml -o $(COVERAGE) -t "cutil test coverage" --num-spaces 4 cutil.info

clean:
	rm -rf $(NAME)
	rm -rf include
	rm -rf lib
	rm -rf $(OBJ)
	rm -rf $(OUT)
	rm -rf $(GCDA)
	rm -rf $(GCNO)
	rm -rf $(GCOV)
	rm -rf *.info

.PHONY: all install uninstall clean test coverage report

