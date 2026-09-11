CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -lcurl

SRCS = main.c core.c parser.c net.c packsync.c
OBJS = $(SRCS:.c=.o)
TARGET = fortune

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
