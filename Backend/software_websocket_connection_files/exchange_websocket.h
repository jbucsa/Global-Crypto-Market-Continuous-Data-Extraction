/*
 * Exchange WebSocket Header
 * 
 * This header file declares functions and global variables used for managing 
 * WebSocket connections to multiple cryptocurrency exchanges.
 * 
 * Functionality:
 *  - Declares `callback_combined()`, a unified WebSocket callback function 
 *    that processes events such as connection, message reception, and disconnection.
 *  - Declares the global `protocols[]` array, which defines WebSocket protocols 
 *    for supported exchanges.
 * 
 * Dependencies:
 *  - libwebsockets: Provides WebSocket functionality.
 * 
 * Usage:
 *  - Included in `exchange_websocket.c` for implementation.
 *  - Included in `main.c` for initializing WebSocket connections.
 * 
 * Created: 3/7/2025
 * Updated: 3/11/2025
 */

#ifndef EXCHANGE_WEBSOCKET_H
#define EXCHANGE_WEBSOCKET_H

#define MAX_EXCHANGE_NAME_LENGTH 32

#include <libwebsockets.h>

typedef struct {
    char price[32];
    char currency[32];
    char time_ms[32]; // Binance specific field to allow for different format
    char timestamp[64];

    char bid[32];
    char ask[32];
    char bid_qty[32];
    char ask_qty[32];

    char open_price[32];
    char high_price[32];
    char low_price[32];

    char close_price[32];
    char volume_24h[32];
    char volume_30d[32];
    char quote_volume[32];

    char symbol[32];
    char last_trade_time[32];
    char last_trade_price[32];
    char last_trade_size[32];

    char trade_id[64];

    char sequence[64];

    char exchange[32];
} TickerData;

/* Callback function for handling WebSocket events. */
int callback_combined(struct lws *wsi, enum lws_callback_reasons reason,
                      void *user, void *in, size_t len);


/* Function to write data to bson file after extracted to struct */
void write_ticker_to_bson(const TickerData *ticker);

/* Global protocols array (defined in exchange_websocket.c) */
extern struct lws_protocols protocols[];

/* Global file pointer for writing market data. */
extern FILE *ticker_data_file;
extern FILE *trades_data_file;

#endif // EXCHANGE_WEBSOCKET_H
