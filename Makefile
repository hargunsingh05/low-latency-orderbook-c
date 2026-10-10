CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c99 -g -Iinclude
OPTFLAGS = -O3

SRC = src/orderbook.c src/main.c
OBJ = $(SRC:.c=.o)
TARGET = orderbook_engine

.PHONY: all clean run debug sanitize

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OPTFLAGS) -o $@ $(OBJ)

src/%.o: src/%.c include/orderbook.h
	$(CC) $(CFLAGS) $(OPTFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

debug: CFLAGS += -O0
debug: clean all

sanitize: CFLAGS += -O1 -fsanitize=address,undefined -fno-omit-frame-pointer
sanitize: clean all
	./$(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)

TEST_TARGET = test_orderbook
TEST_SRC = tests/test_orderbook.c src/orderbook.c

$(TEST_TARGET): $(TEST_SRC) include/orderbook.h
	$(CC) $(CFLAGS) -O0 $(TEST_SRC) -o $(TEST_TARGET)

.PHONY: test
test: $(TEST_TARGET)
	./$(TEST_TARGET)
