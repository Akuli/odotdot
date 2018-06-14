CC ?= cc
CFLAGS += -Wall -Wextra -Wpedantic -std=c99 -Wno-unused-parameter

# TODO: comment this out some day? this is for debugging with gdb and valgrind
CFLAGS += -g

SRC := $(filter-out src/main.c, $(wildcard src/*.c src/objects/*.c src/builtins/*.c))
OBJ := $(SRC:src/%.c=obj/%.o)
CTESTS_SRC := $(wildcard ctests/*.c) $(wildcard ctests/*.h)

all: ö

ö: $(OBJ) src/main.c
	$(CC) -I. $(CFLAGS) src/main.c $(OBJ) -o ö

.PHONY: clean
clean:
	rm -vrf obj ctestsrunner *-compiled ö

misc-compiled/%: misc/%.c $(OBJ)
	mkdir -p $(@D) && $(CC) -o $@ $(OBJ) $(CFLAGS) $< -I.

obj/%.o: src/%.c
	mkdir -p $(@D) && $(CC) -c -o $@ $< $(CFLAGS)

ctestsrunner: $(CTESTS_SRC) $(OBJ)
	$(CC) -I. $(CFLAGS) $(CTESTS_SRC) $(OBJ) -o ctestsrunner

.PHONY: test
test: ö ctestsrunner
	./ctestsrunner
	(for file in ötests/test_*.ö; do ./ö "$$file" && printf . || exit 1; done)
	@echo
	@echo "ok"

.PHONY: testverbose
testverbose: ö ctestsrunner
	./ctestsrunner --verbose
	@echo
	(for file in ötests/test_*.ö; do printf '%-40s  ' "$$file"; ./ö "$$file" && echo "ok" || (echo "returned $$?"; exit 1) || exit 1; done)
	@echo
	@echo "---------------------------"
	@echo "all ötests pass"

.PHONY: iwyu
iwyu:
	for file in $(SRC) $(CTESTS_SRC) src/main.c; do iwyu -I. $$file; done || true

# i wasn't sure what was going on so i used stackoverflow... https://stackoverflow.com/a/42846187
.PRECIOUS: obj/%.o
