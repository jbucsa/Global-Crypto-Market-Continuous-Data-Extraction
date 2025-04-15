/*
 * Exchange Reconnection
 * 
 * This module manages WebSocket reconnection attempts for cryptocurrency 
 * exchanges, ensuring continued data flow in case of connection failures.
 * 
 * Features:
 *  - Tracks retry attempts per exchange.
 *  - Implements exponential backoff with a maximum wait time.
 *  - Triggers reconnection upon disconnection or failure.
 * 
 * Dependencies:
 *  - Standard C libraries (stdio, string, unistd).
 * 
 * Usage:
 *  - Called in `exchange_websocket.c` when a connection is lost.
 *  - Uses `exchange_connect.c` to re-establish WebSocket connections.
 * 
 * Created: 3/11/2025
 * Updated: 4/14/2025
 */

#include "exchange_reconnect.h"
#include "exchange_connect.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

/* Track retry count for each exchange */
ExchangeRetry retry_counts[MAX_EXCHANGES] = {
    {"binance-websocket", 0},
    {"coinbase-websocket", 0},
    {"kraken-websocket", 0},
    {"bitfinex-websocket", 0},
    {"huobi-websocket", 0},
    {"okx-websocket", 0}
};

/* Find retry count index for an exchange */
int get_exchange_index(const char *exchange) {
    for (int i = 0; i < MAX_EXCHANGES; i++) {
        if (strcmp(retry_counts[i].exchange, exchange) == 0) {
            return i;
        }
    }
    return -1;
}

/* Schedule reconnection with per-exchange retry count */
void schedule_reconnect(const char *exchange) {
    int index = get_exchange_index(exchange);
    if (index == -1) {
        printf("[ERROR] Unknown exchange: %s\n", exchange);
        return;
    }

    int wait_time = retry_counts[index].retry_count;
    if (wait_time > 10) wait_time = 10;

    printf("[INFO] Attempting to reconnect to %s in %d seconds...\n", exchange, wait_time);
    sleep(wait_time);
    retry_counts[index].retry_count++;

    if (strcmp(exchange, "binance-websocket") == 0) {
        connect_to_binance();
    } else if (strcmp(exchange, "coinbase-websocket") == 0) {
        connect_to_coinbase();
    } else if (strcmp(exchange, "kraken-websocket") == 0) {
        connect_to_kraken();
    } else if (strcmp(exchange, "bitfinex-websocket") == 0) {
        connect_to_bitfinex();
    } else if (strcmp(exchange, "huobi-websocket") == 0) {
        connect_to_huobi();
    } else if (strcmp(exchange, "okx-websocket") == 0) {
        connect_to_okx();
    }
}
