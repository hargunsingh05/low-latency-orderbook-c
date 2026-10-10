#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include "../include/orderbook.h"

#define NUM_ORDERS 20000

int main(void)
{
    OrderBook *book = create_orderbook();

    if (book == NULL) {
        fprintf(stderr, "Failed to create order book.\n");
        return 1;
    }

    clock_t start = clock();

    for (uint32_t i = 0; i < NUM_ORDERS; i++) {
        double price = 100.0 + (double)(i % 100);

        if (!add_order(book, (uint64_t)i + 1,
                       SIDE_BUY, price, 10)) {
            fprintf(stderr, "Failed to add order %u\n", i);
            free_orderbook(book);
            return 1;
        }
    }

    clock_t end = clock();

    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;

    printf("Order insertion benchmark\n");
    printf("-------------------------\n");
    printf("Orders added: %u\n", (unsigned)NUM_ORDERS);
    printf("Elapsed CPU time: %.6f seconds\n", elapsed);

    if (elapsed > 0.0) {
        printf("Insertion throughput: %.0f orders/second\n",
               NUM_ORDERS / elapsed);
    } else {
        printf("Elapsed time too short for a reliable throughput result.\n");
    }

    free_orderbook(book);
    return 0;
}
