CC = gcc
CFLAGS = -O2 -Wall -Wextra -Isrc
LIBS = -lcurl
SRC = $(wildcard src/*.c)
TARGET = fortune

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
