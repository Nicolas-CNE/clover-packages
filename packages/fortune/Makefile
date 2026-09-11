CC = gcc
CFLAGS = -Wall -Wextra -O2 -Iinclude -Isrc
LDFLAGS = -lcurl

SRCS = src/main.c src/core.c src/db.c src/deps.c src/network.c src/packsync.c src/parser.c src/sync.c
OBJS = $(SRCS:.c=.o)
TARGET = fortune

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
