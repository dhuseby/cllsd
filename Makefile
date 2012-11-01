# recursively build cllsd
#
SHELL=/bin/sh
INSTALL=/usr/bin/install
INSTALL_PROGRAM=$(INSTALL)
INSTALL_DATA=$(INSTALL) -m 644
COVERAGE?=./coverage

CUTIL = cutil
BASEDIRS = src tests
ifneq ($(BUILD_LANG),"")
	BASEDIRS += $(BUILD_LANG)
endif
DIRS = $(CUTIL) $(BASEDIRS)
CDIRS = $(CUTIL) src tests python ruby php
BUILDDIRS = $(DIRS:%=build-%)
INSTALLDIRS = $(DIRS:%=install-%)
UNINSTALLDIRS = $(DIRS:%=uninstall-%)
CLEANDIRS = $(CDIRS:%=clean-%)
TESTDIRS = $(DIRS:%=test-%)
GCOVDIRS = $(BASEDIRS:%=gcov-%)
REPORTDIRS = $(BASEDIRS:%=report-%)
CUTILDIRS = $(CUTIL:%=cutildir-%)

all: $(BUILDDIRS)

$(DIRS): $(BUILDDIRS)

$(BUILDDIRS):
	$(MAKE) -C $(@:build-%=%)

install: $(INSTALLDIRS) all

$(INSTALLDIRS):
	$(MAKE) -C $(@:install-%=%) install

uninstall: $(UNINSTALLDIRS) all

$(UNINSTALLDIRS):
	$(MAKE) -C $(@:uninstall-%=%) uninstall

test: $(TESTDIRS) all

$(TESTDIRS):
	$(MAKE) -C $(@:test-%=%) test

cutildir: $(CUTILDIRS)

$(CUTILDIRS):
	$(MAKE) -C $(@:cutildir-%=%) test

coverage: cutildir $(GCOVDIRS) $(REPORTDIRS)

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
.PHONY: all install uninstall clean test coverage report cutildir

