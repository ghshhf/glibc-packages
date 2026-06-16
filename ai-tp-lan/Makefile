CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -Iinclude
SRCDIR  = src
OBJDIR  = build
TESTDIR = test

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SOURCES))

.PHONY: all clean test

all: libaitp-lan.a

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

libaitp-lan.a: $(OBJECTS)
	ar rcs $@ $^

test: libaitp-lan.a
	$(CC) $(CFLAGS) $(TESTDIR)/test_basic.c -L. -laitp-lan -o test_lan
	./test_lan

clean:
	rm -rf $(OBJDIR) libaitp-lan.a test_lan
