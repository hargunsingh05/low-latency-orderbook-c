#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "../include/orderbook.h"

/*
 * Multi-level limit order book.
 *
 * Price levels are maintained in sorted doubly-linked lists:
 *   - Bids: highest price first.
 *   - Asks: lowest price first.
 *
 * Orders at the same price are matched FIFO.
 *
 * Note: linked-list price-level insertion is O(number of levels).
 * This implementation is not lock-free or a complete exchange feed engine.
 */

static PriceLevel *create_price_level(double price)
{
    PriceLevel *level = malloc(sizeof(*level));

    if (level == NULL) {
        return NULL;
    }

    level->price = price;
    level->total_volume = 0;
    level->order_count = 0;
    level->head = NULL;
    level->tail = NULL;
    level->prev = NULL;
    level->next = NULL;

    return level;
}

static void free_price_level(PriceLevel *level)
{
    if (level == NULL) {
        return;
    }

    Order *order = level->head;

    while (order != NULL) {
        Order *next = order->next;
        free(order);
        order = next;
    }

    free(level);
}

static PriceLevel *find_price_level(
    PriceLevel *root,
    double price)
{
    while (root != NULL) {
        if (root->price == price) {
            return root;
        }

        root = root->next;
    }

    return NULL;
}

/*
 * Insert a price level in price priority order.
 * Bids: descending. Asks: ascending.
 */
static PriceLevel *get_or_create_level(
    OrderBook *book,
    Side side,
    double price)
{
    PriceLevel **root =
        (side == SIDE_BUY) ? &book->best_bid : &book->best_ask;

    PriceLevel *existing = find_price_level(*root, price);

    if (existing != NULL) {
        return existing;
    }

    PriceLevel *level = create_price_level(price);

    if (level == NULL) {
        return NULL;
    }

    PriceLevel *current = *root;
    PriceLevel *previous = NULL;

    while (current != NULL) {
        bool should_insert_before;

        if (side == SIDE_BUY) {
            should_insert_before = price > current->price;
        } else {
            should_insert_before = price < current->price;
        }

        if (should_insert_before) {
            break;
        }

        previous = current;
        current = current->next;
    }

    level->prev = previous;
    level->next = current;

    if (previous != NULL) {
        previous->next = level;
    } else {
        *root = level;
    }

    if (current != NULL) {
        current->prev = level;
    }

    return level;
}

static void unlink_empty_level(
    OrderBook *book,
    Side side,
    PriceLevel *level)
{
    if (level == NULL || level->head != NULL) {
        return;
    }

    PriceLevel **root =
        (side == SIDE_BUY) ? &book->best_bid : &book->best_ask;

    if (level->prev != NULL) {
        level->prev->next = level->next;
    } else {
        *root = level->next;
    }

    if (level->next != NULL) {
        level->next->prev = level->prev;
    }

    free(level);
}

OrderBook *create_orderbook(void)
{
    OrderBook *book = malloc(sizeof(*book));

    if (book == NULL) {
        return NULL;
    }

    book->best_bid = NULL;
    book->best_ask = NULL;
    book->total_orders_processed = 0;

    return book;
}

bool add_order(
    OrderBook *book,
    uint64_t id,
    Side side,
    double price,
    uint32_t qty)
{
    if (book == NULL ||
        (side != SIDE_BUY && side != SIDE_SELL) ||
        !(price > 0.0) ||
        qty == 0) {
        return false;
    }

    /*
     * Prevent duplicate IDs. This implementation searches all levels;
     * a hash table will be a separate optimization.
     */
    PriceLevel *roots[2] = {book->best_bid, book->best_ask};

    for (size_t i = 0; i < 2; ++i) {
        for (PriceLevel *level = roots[i];
             level != NULL;
             level = level->next) {
            for (Order *current = level->head;
                 current != NULL;
                 current = current->next) {
                if (current->order_id == id) {
                    return false;
                }
            }
        }
    }

    PriceLevel *level = get_or_create_level(book, side, price);

    if (level == NULL) {
        return false;
    }

    Order *order = malloc(sizeof(*order));

    if (order == NULL) {
        if (level->order_count == 0) {
            unlink_empty_level(book, side, level);
        }
        return false;
    }

    order->order_id = id;
    order->side = side;
    order->price = price;
    order->qty = qty;
    order->timestamp = 0;
    order->prev = level->tail;
    order->next = NULL;

    if (level->tail != NULL) {
        level->tail->next = order;
    } else {
        level->head = order;
    }

    level->tail = order;
    level->total_volume += qty;
    level->order_count++;

    book->total_orders_processed++;

    return true;
}

/*
 * Match crossing orders using price priority and FIFO within a level.
 * Trades execute at the resting order's price.
 */
void match_orders(OrderBook *book)
{
    if (book == NULL) {
        return;
    }

    while (book->best_bid != NULL &&
           book->best_ask != NULL &&
           book->best_bid->price >= book->best_ask->price) {

        PriceLevel *bid_level = book->best_bid;
        PriceLevel *ask_level = book->best_ask;

        Order *buy = bid_level->head;
        Order *sell = ask_level->head;

        if (buy == NULL || sell == NULL) {
            break;
        }

        uint32_t traded =
            (buy->qty < sell->qty) ? buy->qty : sell->qty;

        if (traded == 0) {
            break;
        }

        buy->qty -= traded;
        sell->qty -= traded;

        bid_level->total_volume -= traded;
        ask_level->total_volume -= traded;

        if (buy->qty == 0) {
            bid_level->head = buy->next;

            if (bid_level->head != NULL) {
                bid_level->head->prev = NULL;
            } else {
                bid_level->tail = NULL;
            }

            free(buy);
            bid_level->order_count--;
        }

        if (sell->qty == 0) {
            ask_level->head = sell->next;

            if (ask_level->head != NULL) {
                ask_level->head->prev = NULL;
            } else {
                ask_level->tail = NULL;
            }

            free(sell);
            ask_level->order_count--;
        }

        unlink_empty_level(book, SIDE_BUY, bid_level);
        unlink_empty_level(book, SIDE_SELL, ask_level);
    }
}

/*
 * Cancel an order by ID.
 * Returns true if the order was found and cancelled.
 */
bool cancel_order(OrderBook *book, uint64_t id)
{
    if (book == NULL) {
        return false;
    }

    PriceLevel *roots[2] = {book->best_bid, book->best_ask};

    for (size_t i = 0; i < 2; ++i) {
        Side side = (i == 0) ? SIDE_BUY : SIDE_SELL;

        for (PriceLevel *level = roots[i];
             level != NULL;
             level = level->next) {

            for (Order *order = level->head;
                 order != NULL;
                 order = order->next) {

                if (order->order_id != id) {
                    continue;
                }

                level->total_volume -= order->qty;

                if (order->prev != NULL) {
                    order->prev->next = order->next;
                } else {
                    level->head = order->next;
                }

                if (order->next != NULL) {
                    order->next->prev = order->prev;
                } else {
                    level->tail = order->prev;
                }

                level->order_count--;
                free(order);

                unlink_empty_level(book, side, level);

                return true;
            }
        }
    }

    return false;
}

void free_orderbook(OrderBook *book)
{
    if (book == NULL) {
        return;
    }

    PriceLevel *level = book->best_bid;

    while (level != NULL) {
        PriceLevel *next = level->next;
        free_price_level(level);
        level = next;
    }

    level = book->best_ask;

    while (level != NULL) {
        PriceLevel *next = level->next;
        free_price_level(level);
        level = next;
    }

    free(book);
}
