#include <assert.h>
#include <stdio.h>
#include "../include/orderbook.h"

static void test_price_priority(void)
{
    OrderBook *book = create_orderbook();
    assert(book != NULL);

    assert(add_order(book, 1, SIDE_BUY, 100.0, 10));
    assert(add_order(book, 2, SIDE_BUY, 101.0, 10));
    assert(add_order(book, 3, SIDE_SELL, 99.0, 7));

    match_orders(book);

    assert(book->best_bid != NULL);
    assert(book->best_bid->price == 101.0);
    assert(book->best_bid->head->qty == 3);
    assert(book->best_bid->next != NULL);
    assert(book->best_bid->next->price == 100.0);
    assert(book->best_ask == NULL);

    free_orderbook(book);
    puts("PASS: price priority");
}

static void test_fifo_priority(void)
{
    OrderBook *book = create_orderbook();
    assert(book != NULL);

    assert(add_order(book, 10, SIDE_BUY, 100.0, 5));
    assert(add_order(book, 11, SIDE_BUY, 100.0, 7));
    assert(add_order(book, 12, SIDE_SELL, 100.0, 6));

    match_orders(book);

    assert(book->best_bid != NULL);
    assert(book->best_bid->head != NULL);
    assert(book->best_bid->head->order_id == 11);
    assert(book->best_bid->head->qty == 6);
    assert(book->best_bid->total_volume == 6);
    assert(book->best_bid->order_count == 1);
    assert(book->best_ask == NULL);

    free_orderbook(book);
    puts("PASS: FIFO priority");
}

static void test_partial_fill(void)
{
    OrderBook *book = create_orderbook();
    assert(book != NULL);

    assert(add_order(book, 20, SIDE_BUY, 105.0, 10));
    assert(add_order(book, 21, SIDE_SELL, 104.0, 4));

    match_orders(book);

    assert(book->best_bid != NULL);
    assert(book->best_bid->head->qty == 6);
    assert(book->best_bid->total_volume == 6);
    assert(book->best_ask == NULL);

    free_orderbook(book);
    puts("PASS: partial fill");
}

static void test_ask_price_priority(void)
{
    OrderBook *book = create_orderbook();
    assert(book != NULL);

    assert(add_order(book, 30, SIDE_SELL, 102.0, 10));
    assert(add_order(book, 31, SIDE_SELL, 101.0, 10));
    assert(add_order(book, 32, SIDE_BUY, 103.0, 7));

    match_orders(book);

    assert(book->best_ask != NULL);
    assert(book->best_ask->price == 101.0);
    assert(book->best_ask->head->qty == 3);
    assert(book->best_ask->next != NULL);
    assert(book->best_ask->next->price == 102.0);
    assert(book->best_bid == NULL);

    free_orderbook(book);
    puts("PASS: ask price priority");
}

static void test_cancellation(void)
{
    OrderBook *book = create_orderbook();
    assert(book != NULL);

    assert(add_order(book, 40, SIDE_BUY, 100.0, 10));
    assert(add_order(book, 41, SIDE_BUY, 100.0, 8));

    assert(cancel_order(book, 40));
    assert(!cancel_order(book, 999));

    assert(book->best_bid != NULL);
    assert(book->best_bid->head->order_id == 41);
    assert(book->best_bid->total_volume == 8);
    assert(book->best_bid->order_count == 1);

    assert(cancel_order(book, 41));
    assert(book->best_bid == NULL);

    free_orderbook(book);
    puts("PASS: cancellation");
}

static void test_invalid_and_duplicate_orders(void)
{
    OrderBook *book = create_orderbook();
    assert(book != NULL);

    assert(!add_order(book, 50, SIDE_BUY, 100.0, 0));
    assert(!add_order(book, 51, SIDE_BUY, -1.0, 10));
    assert(!add_order(book, 52, SIDE_BUY, 0.0, 10));
    assert(!add_order(book, 53, (Side)99, 100.0, 10));

    assert(add_order(book, 54, SIDE_BUY, 100.0, 5));
    assert(!add_order(book, 54, SIDE_SELL, 99.0, 5));
    assert(book->total_orders_processed == 1);

    free_orderbook(book);
    puts("PASS: invalid and duplicate orders");
}

int main(void)
{
    test_price_priority();
    test_fifo_priority();
    test_partial_fill();
    test_ask_price_priority();
    test_cancellation();
    test_invalid_and_duplicate_orders();

    puts("\nAll 6 tests passed.");
    return 0;
}
