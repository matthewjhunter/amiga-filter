# Host test build: compiles Filter's sources on Unix against the stand-in
# Amiga headers in include/, under AddressSanitizer and UBSan.
#
#   make          build and run the tests
#   make cross    compile the real Amiga binaries with m68k-amigaos-gcc (Docker)

CC      ?= gcc
SRC     := ../src
SAN     := -fsanitize=address,undefined -fno-omit-frame-pointer
CFLAGS  := -std=gnu99 -g -O1 $(SAN) -Iinclude -I$(SRC) -D_GNU_SOURCE -Dmain=filter_main
LDFLAGS := $(SAN)

OBJS := Filter.o user.o messagebase.o amiga_stubs.o test_filter.o

.PHONY: test cross clean
test: test_filter
	ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 ./test_filter

test_filter: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

Filter.o: $(SRC)/Filter.c
	$(CC) $(CFLAGS) -c -o $@ $<
user.o: $(SRC)/user.c
	$(CC) $(CFLAGS) -c -o $@ $<
messagebase.o: $(SRC)/messagebase.c
	$(CC) $(CFLAGS) -c -o $@ $<
amiga_stubs.o: amiga_stubs.c include/amiga_stubs.h
	$(CC) $(CFLAGS) -c -o $@ $<
test_filter.o: test_filter.c include/amiga_stubs.h $(SRC)/filter.h
	$(CC) $(CFLAGS) -c -o $@ $<

cross:
	./cross-build.sh

clean:
	rm -f $(OBJS) test_filter
