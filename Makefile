CC = gcc
CFLAGS = -Wall -Wextra -O3 -std=c99 -Iinclude
SRC = src/orderbook.c src/main.c
OBJ = $(SRC:.c=.o)
TARGET = orderbook_engine

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

clean:
	rm -f src/*.o $(TARGET)

.PHONY: all clean
