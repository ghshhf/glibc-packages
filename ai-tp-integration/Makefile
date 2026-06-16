CC = gcc
CFLAGS = -Wall -Wextra -Iinclude

all: test

test: src/ai-tp-integration.c test/test_basic.c
	$(CC) $(CFLAGS) -o test_integration $^
	./test_integration

clean:
	rm -f test_integration
