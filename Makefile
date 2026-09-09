CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O3 -D_GNU_SOURCE -MMD -MP
LDFLAGS = -lcurl

PREFIX ?= /usr/local

SRCS = main.c core.c db.c network.c parser.c deps.c sync.c
OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)
TARGET = fortune

.PHONY: all clean install uninstall

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

-include $(DEPS)

install: $(TARGET)
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(TARGET)

clean:
	rm -f $(OBJS) $(DEPS) $(TARGET)
