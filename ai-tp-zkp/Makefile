CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -Iinclude -I../libaitp-common/include
SRCDIR  = src
OBJDIR  = build
TESTDIR = test

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SOURCES))

.PHONY: all clean test

all: libaitp-zkp.a

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

libaitp-zkp.a: $(OBJECTS)
	ar rcs $@ $^

test: libaitp-zkp.a
	$(CC) $(CFLAGS) $(TESTDIR)/test_basic.c -L. -laitp-zkp -o test_zkp
	./test_zkp

clean:
	rm -rf $(OBJDIR) libaitp-zkp.a test_zkp
