CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -Iinclude -I../libaitp-common/include
SRCDIR  = src
OBJDIR  = build
TESTDIR = test

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SOURCES))

.PHONY: all clean test

all: libaitp-pow.a

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

libaitp-pow.a: $(OBJECTS)
	ar rcs $@ $^

test: libaitp-pow.a
	$(CC) $(CFLAGS) $(TESTDIR)/test_basic.c -L. -laitp-pow -o test_pow
	./test_pow

clean:
	rm -rf $(OBJDIR) libaitp-pow.a test_pow
