CC ?= cc
CFLAGS += -Wall -Wextra -std=c99 -Wno-unused-parameter
TESTARGS ?=

SRC := $(wildcard src/*.c src/objects/*.c)
OBJ := $(SRC:src/%.c=obj/%.o)
CTESTS_SRC := $(wildcard ctests/*.c) $(wildcard ctests/*.h)

.PHONY: all
all: runtests

.PHONY: clean
clean:
	rm -vrf obj runtests

misc-compiled/%: misc/%.c $(OBJ)
	mkdir -p $(@D) && $(CC) -o $@ $(OBJ) $(CFLAGS) $< -I.

obj/%.o: src/%.c
	mkdir -p $(@D) && $(CC) -c -o $@ $< $(CFLAGS)

runtests: $(CTESTS_SRC) $(OBJ)
	$(CC) -I. $(CFLAGS) $(CTESTS_SRC) $(OBJ) -o runtests

.PHONY: iwyu
iwyu:
	for file in $(SRC); do iwyu $$file; done || true

# ctests/utils.h is skipped, see its comments for reason
.PHONY: iwyu-tests
iwyu-tests:
	for file in $(CTESTS_SRC); do iwyu -I. $$file; done || true

# i wasn't sure what's going on so i used stackoverflow... https://stackoverflow.com/a/42846187
.PRECIOUS: obj/%.o
