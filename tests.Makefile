# makefile for running tests, see README

ifdef VALGRIND
VALGRINDOPTS ?= -q --leak-check=full --error-exitcode=1 --errors-for-leak-kinds=all
endif


.PHONY: all
all: ctests $(wildcard ötests/test_*.ö examples/*.ö)
	@echo ok

executables:
	$(MAKE) ö ctestsrunner


ctests: executables
	$(VALGRIND) $(VALGRINDOPTS) ./ctestsrunner

.PHONY: tests-temp
tests-temp:
	rm -rvf tests-temp && mkdir tests-temp

# http://clarkgrubb.com/makefile-style-guide#phony-target-arg
ötests/test_%.ö: tests-temp executables FORCE
	$(VALGRIND) $(VALGRINDOPTS) ./ö $@

examples/%.ö: executables FORCE
	bash -c 'diff <($(VALGRIND) $(VALGRINDOPTS) ./ö $@) <(sed "s:DIRECTORY:$$PWD:g" examples/output/$*.txt)'

FORCE:
