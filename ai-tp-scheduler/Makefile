CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -Iinclude -I../libaitp-common/include
SRCDIR  = src
OBJDIR  = build
TESTDIR = test

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SOURCES))

.PHONY: all clean test

all: libaitp-scheduler.a

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

libaitp-scheduler.a: $(OBJECTS)
	ar rcs $@ $^

test: libaitp-scheduler.a
	$(CC) $(CFLAGS) $(TESTDIR)/test_basic.c -L. -laitp-scheduler -o test_scheduler
	./test_scheduler

clean:
	rm -rf $(OBJDIR) libaitp-scheduler.a test_scheduler
