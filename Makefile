CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c99 -g -Iinclude
OPTFLAGS = -O3

SRC = src/orderbook.c src/main.c
OBJ = $(SRC:.c=.o)
TARGET = orderbook_engine

TEST_TARGET = test_orderbook
TEST_SRC = tests/test_orderbook.c src/orderbook.c

.PHONY: all clean run debug sanitize test

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OPTFLAGS) -o $@ $(OBJ)

src/%.o: src/%.c include/orderbook.h
	$(CC) $(CFLAGS) $(OPTFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

debug:
	$(MAKE) clean
	$(MAKE) OPTFLAGS=-O0 all

sanitize:
	$(MAKE) clean
	$(MAKE) OPTFLAGS="-O1 -fsanitize=address,undefined -fno-omit-frame-pointer" all
	./$(TARGET)

$(TEST_TARGET): $(TEST_SRC) include/orderbook.h
	$(CC) $(CFLAGS) -O0 $(TEST_SRC) -o $(TEST_TARGET)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

clean:
	rm -f $(OBJ) $(TARGET) $(TEST_TARGET)
	rm -rf *.dSYM
