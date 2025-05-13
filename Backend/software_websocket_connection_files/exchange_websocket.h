/*
 * Exchange WebSocket Header
 * 
 * Declares structures and functions for managing real-time WebSocket data 
 * from multiple cryptocurrency exchanges.
 * 
 * Features:
 *  - Unified `TickerData` structure for standardized market fields.
 *  - WebSocket callback handler for message and event processing.
 *  - Subscription builders for different exchange formats.
 *  - BSON writing support for serialized market data.
 * 
 * Dependencies:
 *  - libwebsockets: Manages WebSocket connections.
 *  - stdio.h: Used for file handling of market data output.
 * 
 * Usage:
 *  - Included in `exchange_websocket.c` and `main.c`.
 * 
 * Created: 3/7/2025
 * Updated: 5/8/2025
 */

#ifndef EXCHANGE_WEBSOCKET_H
#define EXCHANGE_WEBSOCKET_H

#define MAX_EXCHANGE_NAME_LENGTH 32

#include <libwebsockets.h>

typedef struct {
    char price[32];
    char currency[32];
    char time_ms[32]; // Binance specific field to allow for different format (this was already here)
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

    // new fields

    char bid_whole[32];
    char ask_whole[32];
    char last_vol[32];
    char vol_today[32];
    char vwap_today[32];
    char low_today[32];
    char vwap_24h[32]; 
    char high_today[32];
    char open_today[32];
    
} TickerData;

typedef struct {
    char exchange[32];
    char currency[32];
    char price[32];
    char size[32];
    char trade_id[64];
    char timestamp[64];
    char market_maker[32];
} TradeData;

/* Function to build the subscription messsages for each exchange */
char* build_subscription_from_file(const char *filename, const char *template_fmt);

char* build_huobi_subscription_from_file(const char *filename);

/* Callback function for handling WebSocket events. */
int callback_combined(struct lws *wsi, enum lws_callback_reasons reason,
                      void *user, void *in, size_t len);


/* Function to write data to bson file after extracted to struct */
void write_ticker_to_bson(const TickerData *ticker);

/* Function to write data to bson file after extracted to struct */
void write_trade_to_bson(const TradeData *trade);

/* Global protocols array (defined in exchange_websocket.c) */
extern struct lws_protocols protocols[];

/* Global file pointer for writing market data. */
extern FILE *ticker_data_file;
extern FILE *trades_data_file;

#endif // EXCHANGE_WEBSOCKET_H
