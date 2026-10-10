#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    SIDE_BUY,
    SIDE_SELL
} Side;

typedef struct Order {
    uint64_t order_id;
    Side side;
    double price;
    uint32_t qty;
    uint64_t timestamp;
    struct Order *prev;
    struct Order *next;
} Order;

typedef struct PriceLevel {
    double price;
    uint32_t total_volume;
    uint32_t order_count;
    Order *head;
    Order *tail;
    struct PriceLevel *prev;
    struct PriceLevel *next;
} PriceLevel;

typedef struct {
    PriceLevel *best_bid;
    PriceLevel *best_ask;
    uint64_t total_orders_processed;
} OrderBook;

OrderBook *create_orderbook(void);
void free_orderbook(OrderBook *book);
bool add_order(OrderBook *book, uint64_t id, Side side,
               double price, uint32_t qty);
bool cancel_order(OrderBook *book, uint64_t id);

void match_orders(OrderBook *book);

#endif
