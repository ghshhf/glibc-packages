CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -Iinclude -I../libaitp-common/include
SRCDIR  = src
OBJDIR  = build
TESTDIR = test

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SOURCES))

.PHONY: all clean test

all: libaitp-crypto.a

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

libaitp-crypto.a: $(OBJECTS)
	ar rcs $@ $^

test: libaitp-crypto.a
	$(CC) $(CFLAGS) $(TESTDIR)/test_basic.c -L. -laitp-crypto -o test_crypto
	./test_crypto

clean:
	rm -rf $(OBJDIR) libaitp-crypto.a test_crypto
