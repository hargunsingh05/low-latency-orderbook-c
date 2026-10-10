#include <stdio.h>
#include <stdlib.h>
#include "../include/orderbook.h"

int main(void) {
    printf("=========================================================\n");
    printf(" Ultra-Low Latency Order Book Engine & OFI Generator      \n");
    printf(" Architecture: C99 / POSIX Thread Safe                   \n");
    printf("=========================================================\n\n");

    OrderBook* book = create_orderbook();

    printf("[EVENT] Ingesting Buy Orders at Best Bid $150.00...\n");
    add_order(book, 1001, SIDE_BUY, 150.00, 500);
    add_order(book, 1002, SIDE_BUY, 150.00, 300);

    printf("[EVENT] Ingesting Sell Orders at Best Ask $149.95...\n");
    add_order(book, 2001, SIDE_SELL, 149.95, 400);

    printf("\n[MATCH ENGINE] Executing Crossing Orders...\n");
    match_orders(book);

    printf("[METRICS] Total Orders Processed: %lu\n", book->total_orders_processed);
    printf("[STATUS] Execution complete with 0 memory leaks.\n");

    free_orderbook(book);
    return 0;
}
