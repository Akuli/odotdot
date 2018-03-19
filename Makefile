CC ?= cc
CFLAGS += -Wall -Wextra -std=c99

SRC := $(wildcard src/*.c)
OBJ := $(SRC:src/%.c=obj/%.o)
CTESTS_SRC := $(wildcard ctests/test_*.c)
CTESTS_EXEC := $(CTESTS_SRC:ctests/test_%.c=ctests-compiled/test_%)

.PHONY: all
all: test

.PHONY: clean
clean:
	rm -vrf obj ctests-compiled

ctests-compiled/test_%: $(OBJ) ctests/utils.h ctests/test_%.c
	mkdir -p ctests-compiled && $(CC) -o $@ $(OBJ) $(CFLAGS) $$(echo $@ | sed s/-compiled//).c -I.

obj/%.o: src/%.c
	mkdir -p obj && $(CC) -c -o $@ $< $(CFLAGS)

test: $(CTESTS_EXEC)
	@(set -e; for file in $(CTESTS_EXEC); do echo $$file; $$file; done; echo "-------- all tests pass --------")

# i wasn't sure what's going on so i used stackoverflow... https://stackoverflow.com/a/42846187
.PRECIOUS: obj/%.o
