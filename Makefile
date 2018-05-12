CC ?= cc
CFLAGS += -Wall -Wextra -Wpedantic -std=c99 -Wno-unused-parameter
TESTARGS ?=

# TODO: comment this out some day? this is for debugging with gdb and valgrind
CFLAGS += -g

SRC := $(filter-out src/main.c, $(wildcard src/*.c src/objects/*.c))
OBJ := $(SRC:src/%.c=obj/%.o)
CTESTS_SRC := $(wildcard ctests/*.c) $(wildcard ctests/*.h)

.PHONY: all
all: ö runtests

ö: $(OBJ) src/main.c
	$(CC) -I. $(CFLAGS) src/main.c $(OBJ) -o ö

.PHONY: clean
clean:
	rm -vrf obj runtests *-compiled ö

misc-compiled/%: misc/%.c $(OBJ)
	mkdir -p $(@D) && $(CC) -o $@ $(OBJ) $(CFLAGS) $< -I.

obj/%.o: src/%.c
	mkdir -p $(@D) && $(CC) -c -o $@ $< $(CFLAGS)

runtests: $(CTESTS_SRC) $(OBJ)
	$(CC) -I. $(CFLAGS) $(CTESTS_SRC) $(OBJ) -o runtests

.PHONY: iwyu
iwyu:
	for file in $(SRC) $(CTESTS_SRC) src/main.c; do iwyu -I. $$file; done || true

# i wasn't sure what was going on so i used stackoverflow... https://stackoverflow.com/a/42846187
.PRECIOUS: obj/%.o
