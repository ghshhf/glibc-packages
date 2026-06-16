CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -Iinclude
SRCDIR  = src
OBJDIR  = build
TESTDIR = test

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SOURCES))

.PHONY: all clean test

all: libaitp-gateway.a

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

libaitp-gateway.a: $(OBJECTS)
	ar rcs $@ $^

test: libaitp-gateway.a
	$(CC) $(CFLAGS) $(TESTDIR)/test_basic.c -L. -laitp-gateway -o test_gateway
	./test_gateway

clean:
	rm -rf $(OBJDIR) libaitp-gateway.a test_gateway
