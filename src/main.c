#include <inttypes.h>
#include <stdio.h>
#include "../include/orderbook.h"

int main(void)
{
    OrderBook *book = create_orderbook();

    if (book == NULL) {
        fprintf(stderr, "Error: failed to create order book.\n");
        return 1;
    }

    printf("============================================\n");
    printf(" Limit Order Book - Matching Engine Demo\n");
    printf(" C99 | Multiple price levels | FIFO matching\n");
    printf("============================================\n\n");

    printf("[EVENT] Adding buy orders...\n");

    if (!add_order(book, 1001, SIDE_BUY, 150.00, 500) ||
        !add_order(book, 1002, SIDE_BUY, 150.00, 300)) {
        fprintf(stderr, "Error: failed to add a buy order.\n");
        free_orderbook(book);
        return 1;
    }

    printf("[EVENT] Adding a crossing sell order...\n");

    if (!add_order(book, 2001, SIDE_SELL, 149.95, 400)) {
        fprintf(stderr, "Error: failed to add the sell order.\n");
        free_orderbook(book);
        return 1;
    }

    printf("[MATCH] Executing crossing orders...\n");
    match_orders(book);

    printf("\n[RESULT] Orders accepted: %" PRIu64 "\n",
           book->total_orders_processed);

    printf("[RESULT] Remaining bid levels:\n");

    for (PriceLevel *level = book->best_bid;
         level != NULL;
         level = level->next) {
        printf("  Price: %.2f | Volume: %" PRIu32
               " | Orders: %" PRIu32 "\n",
               level->price,
               level->total_volume,
               level->order_count);
    }

    printf("[RESULT] Remaining ask levels:\n");

    for (PriceLevel *level = book->best_ask;
         level != NULL;
         level = level->next) {
        printf("  Price: %.2f | Volume: %" PRIu32
               " | Orders: %" PRIu32 "\n",
               level->price,
               level->total_volume,
               level->order_count);
    }

    free_orderbook(book);
    printf("\n[CLEANUP] Order book freed.\n");

    return 0;
}
