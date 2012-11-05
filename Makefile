# recursively build cllsd
#
SHELL=/bin/sh
INSTALL=/usr/bin/install
INSTALL_PROGRAM=$(INSTALL)
INSTALL_DATA=$(INSTALL) -m 644
COVERAGE?=./coverage

DEPS = cutil
SRCDIR = src
TESTDIR = tests
LANGDIRS =
ifneq ($(BUILD_LANG),"")
	LANGDIRS += $(BUILD_LANG)
endif
DIRS = $(DEPS) $(SRCDIR) $(TESTDIR) $(LANGDIRS)
CDIRS = $(DEPS) $(SRCDIR) $(TESTDIR) python ruby php
BUILDDIRS = $(DIRS:%=build-%)
INSTALLDIRS = $(DIRS:%=install-%)
UNINSTALLDIRS = $(DIRS:%=uninstall-%)
CLEANDIRS = $(CDIRS:%=clean-%)
TESTDIRS = $(DIRS:%=test-%)
DEBUGDIRS = $(DEPS:%=debug-%) $(SRCDIR:%=debug-%)
GCOVDIRS = $(BASEDIRS:%=gcov-%)
REPORTDIRS = $(BASEDIRS:%=report-%)
DEPDIRS = $(DEPS:%=depdirs-%)

all: $(BUILDDIRS)

$(DIRS): $(BUILDDIRS)

$(BUILDDIRS):
	$(MAKE) -C $(@:build-%=%)

install: $(INSTALLDIRS)

$(INSTALLDIRS):
	$(MAKE) -C $(@:install-%=%) install

uninstall: $(UNINSTALLDIRS)

$(UNINSTALLDIRS):
	$(MAKE) -C $(@:uninstall-%=%) uninstall

test: $(TESTDIRS)

$(TESTDIRS):
	$(MAKE) -C $(@:test-%=%) test

debug: $(DEBUGDIRS)

$(DEBUGDIRS):
	$(MAKE) -C $(@:debug-%=%) debug

depdirs: $(DEPDIRS)

$(DEPDIRS):
	$(MAKE) -C $(@:depdirs-%=%) test

coverage: $(DEPDIRS) $(GCOVDIRS) $(REPORTDIRS)

$(GCOVDIRS):
	$(MAKE) -C $(@:gcov-%=%) coverage

report: $(REPORTDIRS)

$(REPORTDIRS):
	$(MAKE) -C $(@:report-%=%) report

clean: $(CLEANDIRS)

$(CLEANDIRS):
	$(MAKE) -C $(@:clean-%=%) clean

.PHONY: subdirs $(DIRS)
.PHONY: subdirs $(BUILDDIRS)
.PHONY: subdirs $(INSTALLDIRS)
.PHONY: subdirs $(UNINSTALL)
.PHONY: subdirs $(TESTDIRS)
.PHONY: subdirs $(GCOVDIRS)
.PHONY: subdirs $(REPORTDIRS)
.PHONY: subdirs $(CLEANDIRS)
.PHONY: subdirs $(CUTILDIRS)
.PHONY: all install uninstall clean test debug coverage report depdirs

