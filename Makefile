CC ?= cc
CFLAGS += -Wall -Wextra -Wpedantic -std=c99 -Wno-unused-parameter

# TODO: comment this out some day? this is for debugging with gdb and valgrind
CFLAGS += -g

LDFLAGS += $(shell cat ldflags.txt)

SRC := $(filter-out src/main.c, $(wildcard src/*.c src/objects/*.c src/builtins/*.c))
OBJ := $(SRC:src/%.c=obj/%.o)
HEADERS := $(filter-out src/builtinscode.h, $(wildcard src/*.h) $(wildcard src/objects/*.h)) config.h
CTESTS_SRC := $(wildcard ctests/*.c) $(wildcard ctests/*.h)

# runs when "make" or "make all" is invoked, tests.Makefile shouldn't invoke this
all: ö
	$(MAKE) -f tests.Makefile
	@echo
	@echo
	@echo 'The Ö interpreter was compiled and tested successfully! :D'
	@echo "Run './ö examples/hello.ö' to get a hellö wörld or './ö' for interactive REPL."

ö: src/main.c $(OBJ) $(HEADERS)
	$(CC) -I. $(CFLAGS) $< $(OBJ) -o $@ $(LDFLAGS)

.PHONY: clean
clean:
	rm -vrf obj ctestsrunner *-compiled ö src/builtinscode.h

src/builtinscode.h: misc-compiled/xd src/builtins.ö
	misc-compiled/xd < src/builtins.ö | fold -s > src/builtinscode.h

# right now src/run.c is the only file that uses src/builtinscode.h
src/run.c: src/builtinscode.h

# FIXME: making run.c depend on builtinscode.h doesn't seem to wörk, so we need this
obj/run.o: src/run.c src/builtinscode.h $(HEADERS)
	mkdir -p $(@D) && $(CC) -c -o $@ $< $(CFLAGS)

misc-compiled/%: misc/%.c $(filter-out obj/run.o, $(OBJ))
	mkdir -p $(@D) && $(CC) -o $@ $(OBJ) $(CFLAGS) $< -I.

misc-compiled/xd: misc/xd.c
	mkdir -p $(@D) && $(CC) -o $@ $(CFLAGS) $<

obj/%.o: src/%.c $(HEADERS)
	mkdir -p $(@D) && $(CC) -c -o $@ $< $(CFLAGS)

ctestsrunner: $(CTESTS_SRC) $(OBJ)
	$(CC) -I. $(CFLAGS) $(CTESTS_SRC) $(OBJ) -o ctestsrunner

.PHONY: iwyu
iwyu:
	for file in $(SRC) $(CTESTS_SRC) src/main.c; do iwyu -I. $$file; done || true

# i wasn't sure what was going on so i used stackoverflow... https://stackoverflow.com/a/42846187
.PRECIOUS: obj/%.o
