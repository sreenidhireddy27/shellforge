CC = gcc
CFLAGS = -Wall -Wextra -g

TARGET = shellforge

SRC = src/main.c
OBJ = src/main.o

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

src/main.o: src/main.c
	$(CC) $(CFLAGS) -c src/main.c -o src/main.o

clean:
	rm -f $(OBJ) $(TARGET)
