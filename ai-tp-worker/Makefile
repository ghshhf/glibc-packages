CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -Iinclude -I../libaitp-common/include -I../ai-tp-lan/include
SRCDIR  = src
OBJDIR  = build
TESTDIR = test

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SOURCES))

.PHONY: all clean test

all: libaitp-worker.a

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

libaitp-worker.a: $(OBJECTS)
	ar rcs $@ $^

test: libaitp-worker.a
	$(CC) $(CFLAGS) $(TESTDIR)/test_basic.c -L. -laitp-worker -o test_worker
	./test_worker

clean:
	rm -rf $(OBJDIR) libaitp-worker.a test_worker
