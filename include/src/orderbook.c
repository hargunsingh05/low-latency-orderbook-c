#include <stdio.h>
#include <stdlib.h>
#include "../include/orderbook.h"

OrderBook* create_orderbook(void) {
    OrderBook* book = (OrderBook*)malloc(sizeof(OrderBook));
    if (!book) return NULL;
    book->best_bid = NULL;
    book->best_ask = NULL;
    book->total_orders_processed = 0;
    return book;
}

static PriceLevel* create_price_level(double price) {
    PriceLevel* lvl = (PriceLevel*)malloc(sizeof(PriceLevel));
    if (!lvl) return NULL;
    lvl->price = price;
    lvl->total_volume = 0;
    lvl->order_count = 0;
    lvl->head = NULL;
    lvl->tail = NULL;
    lvl->prev = NULL;
    lvl->next = NULL;
    return lvl;
}

bool add_order(OrderBook* book, uint64_t id, Side side, double price, uint32_t qty) {
    Order* order = (Order*)malloc(sizeof(Order));
    if (!order) return false;
    order->order_id = id;
    order->side = side;
    order->price = price;
    order->qty = qty;
    order->timestamp = 1000;
    order->prev = NULL;
    order->next = NULL;

    PriceLevel** target_root = (side == SIDE_BUY) ? &(book->best_bid) : &(book->best_ask);
    
    if (*target_root == NULL) {
        *target_root = create_price_level(price);
    }

    PriceLevel* lvl = *target_root;
    if (lvl->tail == NULL) {
        lvl->head = order;
        lvl->tail = order;
    } else {
        lvl->tail->next = order;
        order->prev = lvl->tail;
        lvl->tail = order;
    }

    lvl->total_volume += qty;
    lvl->order_count++;
    book->total_orders_processed++;
    return true;
}

void match_orders(OrderBook* book) {
    while (book->best_bid && book->best_ask && (book->best_bid->price >= book->best_ask->price)) {
        PriceLevel* bid_lvl = book->best_bid;
        PriceLevel* ask_lvl = book->best_ask;

        Order* buy_order = bid_lvl->head;
        Order* sell_order = ask_lvl->head;

        if (!buy_order || !sell_order) break;

        uint32_t trade_qty = (buy_order->qty < sell_order->qty) ? buy_order->qty : sell_order->qty;
        buy_order->qty -= trade_qty;
        sell_order->qty -= trade_qty;
        bid_lvl->total_volume -= trade_qty;
        ask_lvl->total_volume -= trade_qty;

        if (buy_order->qty == 0) {
            bid_lvl->head = buy_order->next;
            if (bid_lvl->head) bid_lvl->head->prev = NULL;
            free(buy_order);
            bid_lvl->order_count--;
        }

        if (sell_order->qty == 0) {
            ask_lvl->head = sell_order->next;
            if (ask_lvl->head) ask_lvl->head->prev = NULL;
            free(sell_order);
            ask_lvl->order_count--;
        }
    }
}

void free_orderbook(OrderBook* book) {
    if (!book) return;
    if (book->best_bid) {
        Order* curr = book->best_bid->head;
        while (curr) {
            Order* tmp = curr;
            curr = curr->next;
            free(tmp);
        }
        free(book->best_bid);
    }
    if (book->best_ask) {
        Order* curr = book->best_ask->head;
        while (curr) {
            Order* tmp = curr;
            curr = curr->next;
            free(tmp);
        }
        free(book->best_ask);
    }
    free(book);
}
