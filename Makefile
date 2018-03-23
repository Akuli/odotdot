RUN ?= valgrind --quiet --leak-check=yes
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

ctests-compiled/test_%: ctests/test_%.c ctests/utils.h $(OBJ)
	mkdir -p $$(dirname $@) && $(CC) -o $@ $(OBJ) $(CFLAGS) $< -I.

misc-compiled/%: misc/%.c $(OBJ)
	mkdir -p $$(dirname $@) && $(CC) -o $@ $(OBJ) $(CFLAGS) $< -I.

obj/%.o: src/%.c
	mkdir -p $$(dirname $@) && $(CC) -c -o $@ $< $(CFLAGS)

.PHONY: test
test: $(CTESTS_EXEC)
	@(set -e; for file in $(CTESTS_EXEC); do echo $$file; $(RUN) $$file; done; echo "-------- all tests pass --------")

.PHONY: iwyu
iwyu:
	for file in $(SRC); do iwyu $$file; done || true

# ctests/utils.h is skipped, see its comments for reason
.PHONY: iwyu-tests
iwyu-tests:
	for file in $(CTESTS_SRC); do iwyu -I. $$file; done || true

# i wasn't sure what's going on so i used stackoverflow... https://stackoverflow.com/a/42846187
.PRECIOUS: obj/%.o
