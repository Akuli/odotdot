# makefile for running tests, see README

ifdef VALGRIND
# FIXME: this doesn't seem to work
VALGRINDOPTS ?= -q --leak-check=full --error-exitcode=1 --errors-for-leak-kinds=all
endif


.PHONY: all
all: ctests $(wildcard ötests/test_*.ö examples/*.ö)
	@echo ok

executables:
	make ö ctestsrunner


ctests: executables
	./ctestsrunner

# http://clarkgrubb.com/makefile-style-guide#phony-target-arg
ötests/test_%.ö: executables FORCE
	$(VALGRIND) $(VALGRINDOPTS) ./ö $@

examples/%.ö: executables FORCE
	bash -c 'diff <($(VALGRIND) $(VALGRINDOPTS) ./ö $@) <(sed "s:DIRECTORY:$$PWD:g" examples/output/$*.txt)'

FORCE:
