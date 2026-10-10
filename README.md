# Ultra-Low Latency Limit Order Book & OFI Signal Generator

An in-memory, high-frequency limit order book (LOB) and real-time Order Flow Imbalance (OFI) signal engine implemented in standard C (C99). Designed for low-latency market microstructure research, high-frequency execution simulation, and quantitative trading strategy backtesting.

---

## Technical Features

* **$O(1)$ Order Book Operations:** Constant time order insertions, cancellations, and execution matching using doubly-linked list price levels combined with hash-indexed order lookups.
* **Lock-Free Ring Buffer:** Ingests high-throughput tick data across threads with minimal contention using atomic circular buffers.
* **Order Flow Imbalance (OFI) Calculation:** Computes multi-level OFI metrics across top-5 bid/ask depth in real time:
  $$\text{OFI}_t = \Delta V_t^b - \Delta V_t^a$$
* **Multithreaded Architecture:** Decouples market data feed parsing from signal generation using POSIX threads (`pthreads`) and CPU core affinity pinning.
* **Zero-Memory-Leak Validation:** Strict dynamic memory management tested and validated using Valgrind and GCC AddressSanitizer (`ASan`).

---

## Directory Structure

```text
├── src/
│   ├── orderbook.c    # Core LOB dynamic structures and matching algorithms
│   ├── ofi_signal.c   # Multi-level Order Flow Imbalance calculation engine
│   └── main.c         # High-throughput market data feed simulator
├── include/
│   ├── orderbook.h    # Data models (Order, PriceLevel, OrderBook)
│   └── ofi_signal.h   # OFI engine function declarations
├── Makefile           # GCC build rules with -O3 optimization flags
└── README.md          # System documentation
