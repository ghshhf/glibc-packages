CC = gcc
CFLAGS = -Wall -Wextra -Iinclude
AR = ar
ARFLAGS = rcs

LIB = libaitp-common.a
OBJ = libaitp-common.o

all: $(LIB)

$(OBJ): src/libaitp-common.c include/libaitp-common.h
	$(CC) $(CFLAGS) -c -o $@ src/libaitp-common.c

$(LIB): $(OBJ)
	$(AR) $(ARFLAGS) $@ $(OBJ)

test: $(LIB)
	$(CC) $(CFLAGS) -o test_common test/test_basic.c -L. -laitp-common
	./test_common

clean:
	rm -f $(OBJ) $(LIB) test_common

.PHONY: all test clean
