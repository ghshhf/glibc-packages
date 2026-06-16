CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2 -fPIC
LDFLAGS ?= 

PREFIX ?= /usr/local
INCDIR ?= $(PREFIX)/include
LIBDIR ?= $(PREFIX)/lib

SRCDIR = src
INCDIR_LOCAL = include

SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS = $(SRCS:.c=.o)

TARGET = libai-tp-nat.so
TARGET_STATIC = libai-tp-nat.a

.PHONY: all clean install uninstall test

all: $(TARGET) $(TARGET_STATIC)

$(TARGET): $(OBJS)
	$(CC) -shared -o $@ $^ $(LDFLAGS)

$(TARGET_STATIC): $(OBJS)
	ar rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -I$(INCDIR_LOCAL) -c $< -o $@

test: $(TARGET)
	$(CC) $(CFLAGS) -I$(INCDIR_LOCAL) test/test_basic.c -L. -lai-tp-nat -o test_basic
	./test_basic

install: $(TARGET) $(TARGET_STATIC)
	install -d $(INCDIR)
	install -d $(LIBDIR)
	install -m 644 $(INCDIR_LOCAL)/*.h $(INCDIR)/
	install -m 755 $(TARGET) $(LIBDIR)/
	install -m 644 $(TARGET_STATIC) $(LIBDIR)/
	ldconfig || true

uninstall:
	rm -f $(INCDIR)/ai-tp-nat.h
	rm -f $(LIBDIR)/$(TARGET)
	rm -f $(LIBDIR)/$(TARGET_STATIC)
	ldconfig || true

clean:
	rm -f $(SRCDIR)/*.o
	rm -f $(TARGET) $(TARGET_STATIC)
	rm -f test_basic
