# you can also set RUN, e.g. RUN=valgrind runs tests with valgrind
CC ?= cc
CFLAGS += -Wall -Wextra -std=c99 -Wno-unused-parameter

SRC := $(wildcard src/*.c)
OBJ := $(SRC:src/%.c=obj/%.o)
CTESTS_SRC := $(wildcard ctests/test_*.c)
CTESTS_EXEC := $(CTESTS_SRC:ctests/test_%.c=ctests-compiled/test_%)

.PHONY: all
all: $(CTESTS_EXEC)

.PHONY: clean
clean:
	rm -vrf obj ctests-compiled

ctests-compiled/test_%: ctests/test_%.c src/%.c src/%.h ctests/utils.h $(OBJ)
	mkdir -p ctests-compiled && $(CC) -o $@ $(OBJ) $(CFLAGS) $< -I.

obj/%.o: src/%.c
	mkdir -p obj && $(CC) -c -o $@ $< $(CFLAGS)

test: $(CTESTS_EXEC)
	@(set -e; for file in $(CTESTS_EXEC); do echo $$file; $(RUN) $$file; done; echo "-------- all tests pass --------")

# i wasn't sure what's going on so i used stackoverflow... https://stackoverflow.com/a/42846187
.PRECIOUS: obj/%.o
