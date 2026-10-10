#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "../include/orderbook.h"

/*
 * Multi-level limit order book.
 * Bids: highest price first.
 * Asks: lowest price first.
 * Orders at each price level: FIFO.
 * Order IDs: hash table for faster lookup.
 */

/* ---------- Order ID hash table ---------- */

static size_t hash_order_id(uint64_t id, size_t table_size)
{
    id ^= id >> 33;
    id *= UINT64_C(0xff51afd7ed558ccd);
    id ^= id >> 33;
    id *= UINT64_C(0xc4ceb9fe1a85ec53);
    id ^= id >> 33;

    return (size_t)(id % table_size);
}

static Order *find_order_by_id(const OrderBook *book, uint64_t id)
{
    size_t index = hash_order_id(id, book->order_id_table_size);

    for (Order *order = book->order_id_table[index];
         order != NULL;
         order = order->hash_next) {
        if (order->order_id == id) {
            return order;
        }
    }

    return NULL;
}

static void insert_order_hash(OrderBook *book, Order *order)
{
    size_t index = hash_order_id(
        order->order_id, book->order_id_table_size);

    order->hash_next = book->order_id_table[index];
    book->order_id_table[index] = order;
}

static void remove_order_hash(OrderBook *book, uint64_t id)
{
    size_t index = hash_order_id(id, book->order_id_table_size);
    Order **current = &book->order_id_table[index];

    while (*current != NULL) {
        if ((*current)->order_id == id) {
            Order *removed = *current;
            *current = removed->hash_next;
            removed->hash_next = NULL;
            return;
        }

        current = &(*current)->hash_next;
    }
}

/* ---------- Price levels ---------- */

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

static PriceLevel *find_price_level(PriceLevel *root, double price)
{
    while (root != NULL) {
        if (root->price == price) {
            return root;
        }

        root = root->next;
    }

    return NULL;
}

static PriceLevel *get_or_create_level(
    OrderBook *book, Side side, double price)
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
        bool insert_before =
            (side == SIDE_BUY)
                ? price > current->price
                : price < current->price;

        if (insert_before) {
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
    OrderBook *book, Side side, PriceLevel *level)
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

/* ---------- Order book lifecycle ---------- */

OrderBook *create_orderbook(void)
{
    OrderBook *book = malloc(sizeof(*book));

    if (book == NULL) {
        return NULL;
    }

    book->best_bid = NULL;
    book->best_ask = NULL;
    book->total_orders_processed = 0;
    book->order_id_table_size = ORDER_ID_TABLE_SIZE;

    book->order_id_table = calloc(
        book->order_id_table_size,
        sizeof(*book->order_id_table));

    if (book->order_id_table == NULL) {
        free(book);
        return NULL;
    }

    return book;
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

    free(book->order_id_table);
    free(book);
}

/* ---------- Add order ---------- */

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

    if (find_order_by_id(book, id) != NULL) {
        return false;
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
    order->timestamp = book->total_orders_processed;
    order->prev = level->tail;
    order->next = NULL;
    order->hash_next = NULL;

    if (level->tail != NULL) {
        level->tail->next = order;
    } else {
        level->head = order;
    }

    level->tail = order;
    level->total_volume += qty;
    level->order_count++;

    insert_order_hash(book, order);
    book->total_orders_processed++;

    return true;
}

/* ---------- Match orders ---------- */

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

        uint64_t buy_id = buy->order_id;
        uint64_t sell_id = sell->order_id;

        double execution_price =
            (buy->timestamp < sell->timestamp)
                ? buy->price
                : sell->price;

        printf("[TRADE] Buy ID: %llu | Sell ID: %llu\n",
               (unsigned long long)buy_id,
               (unsigned long long)sell_id);
        printf("        Price: %.2f | Quantity: %u\n",
               execution_price, traded);

        buy->qty -= traded;
        sell->qty -= traded;

        bid_level->total_volume -= traded;
        ask_level->total_volume -= traded;

        if (buy->qty == 0) {
            remove_order_hash(book, buy_id);

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
            remove_order_hash(book, sell_id);

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

/* ---------- Cancel order ---------- */

bool cancel_order(OrderBook *book, uint64_t id)
{
    if (book == NULL) {
        return false;
    }

    Order *order = find_order_by_id(book, id);

    if (order == NULL) {
        return false;
    }

    Side side = order->side;
    PriceLevel *level =
        find_price_level(
            (side == SIDE_BUY) ? book->best_bid : book->best_ask,
            order->price);

    if (level == NULL) {
        return false;
    }

    remove_order_hash(book, id);
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
