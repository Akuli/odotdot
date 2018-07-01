CC ?= cc
CFLAGS += -Wall -Wextra -Wpedantic -std=c99 -Wno-unused-parameter

# TODO: comment this out some day? this is for debugging with gdb and valgrind
CFLAGS += -g

SRC := $(filter-out src/main.c, $(wildcard src/*.c src/objects/*.c src/builtins/*.c))
OBJ := $(SRC:src/%.c=obj/%.o)
CTESTS_SRC := $(wildcard ctests/*.c) $(wildcard ctests/*.h)

# runs when "make" or "make all" is invoked, tests.Makefile shouldn't invoke this
all: ö
	make -f tests.Makefile
	@echo
	@echo
	@echo 'The Ö interpreter was compiled and tested successfully! :D'

ö: $(OBJ) src/main.c
	$(CC) -I. $(CFLAGS) src/main.c $(OBJ) -o ö

.PHONY: clean
clean:
	rm -vrf obj ctestsrunner *-compiled ö src/builtinscode.h

# xxd seems to come from vim-common on ubuntu, so i think pretty much everyone have it
# "apt show vim-common" says that even the most minimal vim packages need it
# don't get me wrong... i'm not a vim fan!
src/builtinscode.h: misc-compiled/xd
	misc-compiled/xd < src/builtins.ö > src/builtinscode.h

# right now src/run.c is the only file that uses src/builtinscode.h
src/run.c: src/builtinscode.h

misc-compiled/%: misc/%.c $(filter-out obj/run.o, $(OBJ))
	mkdir -p $(@D) && $(CC) -o $@ $(OBJ) $(CFLAGS) $< -I.

misc-compiled/xd: misc/xd.c
	mkdir -p $(@D) && $(CC) -o $@ $(CFLAGS) $<

obj/%.o: src/%.c
	mkdir -p $(@D) && $(CC) -c -o $@ $< $(CFLAGS)

ctestsrunner: $(CTESTS_SRC) $(OBJ)
	$(CC) -I. $(CFLAGS) $(CTESTS_SRC) $(OBJ) -o ctestsrunner

.PHONY: iwyu
iwyu:
	for file in $(SRC) $(CTESTS_SRC) src/main.c; do iwyu -I. $$file; done || true

# i wasn't sure what was going on so i used stackoverflow... https://stackoverflow.com/a/42846187
.PRECIOUS: obj/%.o
