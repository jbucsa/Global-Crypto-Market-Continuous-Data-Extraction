/*
 * Crypto Exchange WebSocket
 * 
 * This program connects to multiple cryptocurrency exchanges via WebSocket APIs
 * to receive real-time market data, extract relevant information, and log both 
 * ticker prices and trade prices along with timestamps to separate .json files. 
 * 
 * Supported Exchanges:
 *  - Binance (Ticker + Trade)
 *  - Coinbase (Ticker + Trade)
 *  - Kraken (Ticker)
 *  - Bitfinex (Ticker)
 *  - Huobi (Ticker)
 *  - OKX (Ticker)
 * 
 * The program uses the libwebsockets library to establish WebSocket connections.
 * It implements callbacks for handling WebSocket events such as connection
 * establishment, message reception, and disconnection.
 * 
 * Features:
 *  - Converts Binance millisecond timestamps to ISO 8601 format.
 *  - Extracts and logs ticker price and trade price data from JSON messages.
 *  - Supports multiple WebSocket connections concurrently.
 *  - Writes log data in separate JSON files for tickers and trades.
 * 
 * Dependencies:
 *  - libwebsockets : Links the libwebsockets library, required for establishing 
 *    and handling WebSocket connections with exchanges.
 *  - jansson : Links the Jansson library, which is used for handling JSON data.
 *  - lm : Links the math library (libm), needed for mathematical operations 
 *    (used for price comparisons).
 *  - Standard C libraries (stdio, stdlib, string, time, sys/time).
 * 
 * Usage (See README for compilation instructions):
 *    Run the program:
 *      "./crypto_ws"
 * 
 * Created: 3/7/2025
 * Updated: 4/14/2025
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
    
    init_json_buffers();

    // connect_to_binance();
    // connect_to_coinbase();
    int total_symbols = count_symbols_in_file("huobi_currency_ids.txt");
    int num_chunks = (total_symbols + 99) / 100;
    for (int i = 0; i < num_chunks; i++) {
        connect_to_huobi(i);
    }    
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
