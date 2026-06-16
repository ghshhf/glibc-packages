CC=gcc
CFLAGS=-Wall -O2

all:
  $(CC) $(CFLAGS) test/test_basic.c src/ai-tp-sync.c -Iinclude -o test_basic

clean:
  rm -f test_basic

test: all
  ./test_basic
