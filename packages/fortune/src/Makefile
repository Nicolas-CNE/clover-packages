# Variables
CC = gcc
CFLAGS = -Wall
LIBS = -lcurl
TARGET = fortune
SRC = src/fortune.c

# Compilation
all:
	$(CC) $(SRC) -o $(TARGET) $(CFLAGS) $(LIBS)

# Install (requires sudo or root)
install:
	cp $(TARGET) /usr/bin/$(TARGET)

# clean compiled files
clean:
	rm -f $(TARGET)
