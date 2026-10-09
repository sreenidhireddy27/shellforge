CC = gcc
CFLAGS = -Wall -Wextra -g -std=gnu11 -Iinclude
SRCS = src/main.c src/builtins.c src/executor.c
OBJS = $(SRCS:.c=.o)
TARGET = shellforge

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) src/*.o *.o

.PHONY: all clean
