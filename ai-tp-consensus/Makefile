CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -Iinclude -I../libaitp-common/include
SRCDIR  = src
OBJDIR  = build
TESTDIR = test

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SOURCES))

.PHONY: all clean test

all: libaitp-consensus.a

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

libaitp-consensus.a: $(OBJECTS)
	ar rcs $@ $^

test: libaitp-consensus.a
	$(CC) $(CFLAGS) $(TESTDIR)/test_basic.c -L. -laitp-consensus -o test_consensus
	./test_consensus

clean:
	rm -rf $(OBJDIR) libaitp-consensus.a test_consensus
