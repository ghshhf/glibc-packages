CC = gcc
CFLAGS = -Wall -Wextra -Iinclude

all: test

test: src/ai-tp-address.c test/test_basic.c
	$(CC) $(CFLAGS) -o test_address $^
	./test_address

clean:
	rm -f test_address
