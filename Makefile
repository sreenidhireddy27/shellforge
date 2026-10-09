CC = gcc
CFLAGS = -Wall -Wextra -g -std=gnu11

TARGET = shellforge
SRC = src/main.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET) *.o src/*.o

.PHONY: all clean
