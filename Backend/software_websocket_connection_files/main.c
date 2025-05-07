/*
 * Crypto Exchange WebSocket
 * 
 * This program connects to multiple cryptocurrency exchanges via WebSocket APIs
 * to receive real-time market data, extract relevant information, and log both 
 * ticker prices and trade prices along with timestamps to separate .json files. 
 * 
 * Supported Exchanges:
 *  - Binance    (Ticker)
 *  - Coinbase   (Ticker)
 *  - Kraken     (Ticker)
 *  - Huobi      (Ticker)
 *  - OKX        (Ticker)
 *  - Bitfinex   (Planned)
 * 
 * The program uses the libwebsockets library to establish and manage 
 * WebSocket connections. It implements event-driven callbacks to handle 
 * WebSocket events such as connection establishment, message reception, 
 * ping/pong, and disconnection.
 * 
 * Features:
 *  - Extracts and logs ticker and trade price data from incoming JSON messages.
 *  - Converts Binance millisecond timestamps to ISO 8601 format.
 *  - Supports multiple concurrent WebSocket connections.
 *  - Automatic reconnection with exponential backoff on connection failures.
 *  - Periodic health monitoring for each exchange's connection.
 *  - Logs data into separate `.json` and `.bson` files for tickers and trades.
 * 
 * Dependencies:
 *
 *  External Libraries:
 *  -------------------
 *  - libwebsockets (`-lwebsockets`)
 *      Required for managing WebSocket connections, handling protocols, and asynchronous messaging.
 * 
 *  - jansson (`-ljansson`)
 *      Used for parsing, reading, and constructing JSON data structures (e.g., market data responses).
 * 
 *  - libbson (`-lbson-1.0`)
 *      Used to serialize ticker/trade data into BSON format for efficient storage and future analysis.
 * 
 *  - zlib (`-lz`)
 *      Used for decompressing WebSocket message payloads (e.g., Huobi uses gzip).
 * 
 *  - libm (`-lm`)
 *      Standard math library used in calculations (e.g., float handling, price comparisons).
 * 
 *  - libcurl (`-lcurl`)
 *      Required to fetch product ID lists from exchange REST APIs before WebSocket subscriptions.
 * 
 *  Standard C Libraries:
 *  ---------------------
 *  - `stdio.h`      : File I/O and standard output.
 *  - `stdlib.h`     : Memory allocation, process control.
 *  - `string.h`     : String manipulation and comparison.
 *  - `time.h` / `sys/time.h` : Timestamping and formatting.
 *  - `unistd.h`     : Sleep/delay and POSIX API usage.
 *  - `pthread.h`    : Used for running background health monitoring threads.
 *
 *  Notes:
 *  - Make sure all libraries are installed and discoverable via your system's compiler/linker path.
 *  - For Debian/Ubuntu: use `apt install libwebsockets-dev libjansson-dev libcurl4-openssl-dev libbson-1.0-0 zlib1g-dev`
 * 
 * Usage:
 *    See README for build instructions.
 *    Run the program:
 *        ./crypto_ws
 * 
 * Created:  3/7/2025
 * Updated:  5/4/2025
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libwebsockets.h>
#include <errno.h>

#include "exchange_websocket.h"
#include "exchange_connect.h"
#include "utils.h"

/* External declaration of WebSocket protocols */
extern struct lws_protocols protocols[];

/* Global WebSocket context */
struct lws_context *context;

/* Global WebSocket context */
void start_health_monitor(void);

int main() {
    printf("[INFO] Starting Crypto WebSocket Data Logger...\n");

    struct lws_context_creation_info context_info;
    memset(&context_info, 0, sizeof(context_info));
    context_info.port = CONTEXT_PORT_NO_LISTEN;
    context_info.protocols = protocols;
    context_info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;

    context = lws_create_context(&context_info);
    if (!context) {
        printf("[ERROR] Failed to create WebSocket context\n");
        return -1;
    }

    ticker_data_file = fopen("ticker_output_data.json", "a");
    if (!ticker_data_file) {
        printf("[ERROR] Failed to open ticker log file\n");
        lws_context_destroy(context);
        return -1;
    }

    trades_data_file = fopen("trades_output_data.json", "a");
    if (!trades_data_file) {
        printf("[ERROR] Failed to open trades log file\n");
        lws_context_destroy(context);
        return -1;
    }
    
    // Start JSON files
    init_json_buffers();

    // Start connection health tracking
    start_health_monitor();

    // Connect to exchanges
    connect_to_binance();
    // connect_to_coinbase();
    // int total_symbols = count_symbols_in_file("currency_text_files/huobi_currency_ids.txt");
    // int num_chunks = (total_symbols + 99) / 100;
    // for (int i = 0; i < num_chunks; i++) {
    //     connect_to_huobi(i);
    // }    
    // connect_to_kraken();
    // connect_to_okx();

    // connect_to_bitfinex();

    printf("[INFO] All WebSocket connections initialized. Listening for data...\n");

    // Event loop: Handles incoming WebSocket messages and reconnections
    while (lws_service(context, 1000) >= 0) {}

    printf("[INFO] Cleaning up WebSocket context...\n");
    flush_buffer_to_file("ticker_output_data.json", ticker_buffer);
    flush_buffer_to_file("trades_output_data.json", trades_buffer);
    fclose(ticker_data_file);
    fclose(trades_data_file);
    lws_context_destroy(context);

    return 0;
}
