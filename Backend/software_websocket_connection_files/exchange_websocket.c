/*
* Exchange WebSocket
* 
* This module handles WebSocket connections to multiple cryptocurrency exchanges.
* It implements a unified callback function (`callback_combined`) to manage 
* WebSocket events, including connection establishment, data reception, and 
* disconnection.
* 
* Features:
*  - Establishes and manages WebSocket connections for real-time market data.
*  - Processes JSON responses to extract relevant price and timestamp data.
*  - Logs received data for further processing.
*  - Implements a shared protocol array (`protocols[]`) for handling 
*    exchange-specific WebSocket interactions.
* 
* Dependencies:
*  - libwebsockets: Handles WebSocket connections.
*  - jansson: Parses JSON data.
*  - Standard C libraries (stdio, stdlib, string).
* 
* This module is used within `main.c`, where the WebSocket connections are
* initialized and managed in an event-driven loop.
* 
* Created: 3/7/2025
* Updated: 3/12/2025
*/

#include "exchange_websocket.h"
#include "json_parser.h"
#include "utils.h"
#include "exchange_connect.h"
#include "exchange_reconnect.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* Unified Callback for all exchanges */
int callback_combined(struct lws *wsi, enum lws_callback_reasons reason,
    void *user __attribute__((unused)), void *in, size_t len) {
    const struct lws_protocols *protocol_struct = lws_get_protocol(wsi);
    const char *protocol = (protocol_struct) ? protocol_struct->name : "unknown";
    
    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED: {
            printf("[INFO] %s WebSocket Connection Established!\n", protocol);
            const char *subscribe_msg = NULL;
            if (strcmp(protocol, "binance-websocket") == 0) {
                subscribe_msg =
                    "{\"method\": \"SUBSCRIBE\", \"params\": ["
                    "\"btcusdt@ticker\", \"btcusdt@trade\", "
                    "\"ethusdt@ticker\", \"ethusdt@trade\", "
                    "\"adausdt@ticker\", \"adausdt@trade\""
                    "], \"id\": 1}";
            }
            else if (strcmp(protocol, "coinbase-websocket") == 0) {
                subscribe_msg =
                    "{\"type\": \"subscribe\", \"channels\": ["
                    "{ \"name\": \"ticker\", \"product_ids\": [\"BTC-USD\", \"ETH-USD\", \"ADA-USD\"] },"
                    "{ \"name\": \"matches\", \"product_ids\": [\"BTC-USD\", \"ETH-USD\", \"ADA-USD\"] }"
                    "]}";
            }
            else if (strcmp(protocol, "kraken-websocket") == 0) {
                subscribe_msg =
                    "{\"event\": \"subscribe\", \"pair\": [\"XBT/USD\",\"ETH/USD\",\"ADA/USD\"], \"subscription\": {\"name\": \"ticker\"}}";
            }
            else if (strcmp(protocol, "bitfinex-websocket") == 0) {
                subscribe_msg =
                    "{\"event\": \"subscribe\", \"channel\": \"ticker\", \"symbol\": \"tBTCUSD\"}";
            }
            else if (strcmp(protocol, "huobi-websocket") == 0) {
                subscribe_msg =
                    "{\"sub\": \"market.btcusdt.ticker\", \"id\": \"huobi_ticker\"}";
            }
            else if (strcmp(protocol, "okx-websocket") == 0) {
                subscribe_msg =
                    "{\"op\": \"subscribe\", \"args\": [{\"channel\": \"tickers\", \"instId\": \"BTC-USDT\"}]}";
            }
            
            if (subscribe_msg) {
                size_t msg_len = strlen(subscribe_msg);
                unsigned char *buf = malloc(LWS_PRE + msg_len);
                if (!buf) {
                    printf("[ERROR] Memory allocation failed for %s message\n", protocol);
                    return -1;
                }
                memcpy(buf + LWS_PRE, subscribe_msg, msg_len);
                int bytes_sent = lws_write(wsi, buf + LWS_PRE, msg_len, LWS_WRITE_TEXT);
                if (bytes_sent < 0)
                    printf("[ERROR] Failed to send %s subscription message\n", protocol);
                else
                    printf("[INFO] Sent subscription message to %s\n", protocol);
                free(buf);
            }
            
            /* Reset retry count on successful connection */
            {
                int index = get_exchange_index(protocol);
                if (index != -1) {
                    retry_counts[index].retry_count = 0;
                }
            }
            printf("[INFO] %s WebSocket Connection Established! Retry count reset.\n", protocol);
            break;
        }
    
        // (Comment in printf statements below to see full ticker outputs in terminal)
        case LWS_CALLBACK_CLIENT_RECEIVE:
            if (strcmp(protocol, "binance-websocket") == 0) {
                // printf("[DATA][Binance] %.*s\n", (int)len, (char *)in);
                if (strstr((char *)in, "\"e\":\"trade\"")) {
                    char trade_price[32] = {0}, trade_size[32] = {0}, trade_time[32] = {0}, currency[32] = {0}, timestamp[64] = {0};
                    if (extract_price((char *)in, "\"E\":", trade_time, sizeof(trade_time)) &&
                        extract_price((char *)in, "\"s\":\"", currency, sizeof(currency)) &&
                        extract_price((char *)in, "\"p\":\"", trade_price, sizeof(trade_price)) &&
                        extract_price((char *)in, "\"q\":\"", trade_size, sizeof(trade_size))) {
                        convert_binance_timestamp(timestamp, sizeof(timestamp), trade_time);
                        // printf("[TRADE] Binance | %s | Price: %s | Size: %s\n", currency, trade_price, trade_size);
                        log_trade_price(timestamp, "Binance", currency, trade_price, trade_size);
                    }
                }
                else {
                    char price[32] = {0}, currency[32] = {0}, time_ms[32] = {0}, timestamp[64] = {0};
                    if (extract_price((char *)in, "\"E\":", time_ms, sizeof(time_ms)) &&
                        extract_price((char *)in, "\"s\":\"", currency, sizeof(currency)) &&
                        extract_price((char *)in, "\"c\":\"", price, sizeof(price))) {
                        convert_binance_timestamp(timestamp, sizeof(timestamp), time_ms);
                        // printf("[TICKER] Binance | %s | Price: %s\n", currency, price);
                        log_ticker_price(timestamp, "Binance", currency, price);
                    }
                }
            }
            else if (strcmp(protocol, "coinbase-websocket") == 0) {
                // printf("[DATA][Coinbase] %.*s\n", (int)len, (char *)in);
                if (strstr((char *)in, "\"type\":\"match\"") && !strstr((char *)in, "\"type\":\"last_match\"")) {
                    char trade_price[32] = {0}, trade_size[32] = {0}, timestamp[64] = {0}, currency[32] = {0};
                    if (extract_price((char *)in, "\"time\":\"", timestamp, sizeof(timestamp)) &&
                        extract_price((char *)in, "\"product_id\":\"", currency, sizeof(currency)) &&
                        extract_price((char *)in, "\"price\":\"", trade_price, sizeof(trade_price)) &&
                        extract_price((char *)in, "\"size\":\"", trade_size, sizeof(trade_size))) {
                        // printf("[TRADE] Coinbase | %s | Price: %s | Size: %s\n", currency, trade_price, trade_size);
                        log_trade_price(timestamp, "Coinbase", currency, trade_price, trade_size);
                    }
                }
                else if (strstr((char *)in, "\"type\":\"ticker\"")) {
                    char price[32] = {0}, currency[32] = {0}, timestamp[64] = {0};
                    if (extract_price((char *)in, "\"time\":\"", timestamp, sizeof(timestamp)) &&
                        extract_price((char *)in, "\"product_id\":\"", currency, sizeof(currency)) &&
                        extract_price((char *)in, "\"price\":\"", price, sizeof(price))) {
                        // printf("[TICKER] Coinbase | %s | Price: %s\n", currency, price);
                        log_ticker_price(timestamp, "Coinbase", currency, price);
                    }
                }
            }
            else if (strcmp(protocol, "kraken-websocket") == 0) {
                if (strstr((char *)in, "\"event\":\"heartbeat\"")) {
                    return 0;
                }
                // printf("[TICKER][Kraken] %.*s\n", (int)len, (char *)in);
                {
                    char price[32] = {0}, currency[32] = {0}, timestamp[32] = {0};
                    if (extract_price((char *)in, "\"c\":[\"", price, sizeof(price))) {
                        const char *last_quote = strrchr((char *)in, '"');
                        if (last_quote) {
                            const char *start = last_quote - 1;
                            while (start > (char *)in && *start != '"') {
                                start--;
                            }
                            start++;
                            size_t len = last_quote - start;
                            if (len < sizeof(currency)) {
                                strncpy(currency, start, len);
                                currency[len] = '\0';
                            }
                        }
                        get_timestamp(timestamp, sizeof(timestamp));
                        log_ticker_price(timestamp, "Kraken", currency, price);
                    }
                }
            }
            else if (strcmp(protocol, "bitfinex-websocket") == 0) {
                if (strstr((char *)in, "\"hb\"")) {
                    return 0;
                }
                // printf("[TICKER][Bitfinex] %.*s\n", (int)len, (char *)in);
                {
                    char price[32] = {0}, timestamp[32] = {0};
                    if (extract_bitfinex_price((char *)in, price, sizeof(price))) {
                        get_timestamp(timestamp, sizeof(timestamp));
                        log_ticker_price(timestamp, "Bitfinex", "tBTCUSD", price);
                    }
                }
            }
            else if (strcmp(protocol, "huobi-websocket") == 0) {
                char decompressed[8192] = {0};
                int decompressed_len = decompress_gzip((char *)in, len, decompressed, sizeof(decompressed));
                if (decompressed_len > 0) {
                    // printf("[TICKER][Huobi] %.*s\n", decompressed_len, decompressed);

                    /* Handle Huobi ping-pong */
                    char ping_value[32] = {0};
                    if (extract_numeric(decompressed, "\"ping\":", ping_value, sizeof(ping_value))) {
                        char pong_msg[64];
                        snprintf(pong_msg, sizeof(pong_msg), "{\"pong\": %s}", ping_value);
                        unsigned char *buf = malloc(LWS_PRE + strlen(pong_msg));
                        if (!buf) {
                            printf("[ERROR] Memory allocation failed for Huobi pong\n");
                            return -1;
                        }
                        memcpy(buf + LWS_PRE, pong_msg, strlen(pong_msg));
                        lws_write(wsi, buf + LWS_PRE, strlen(pong_msg), LWS_WRITE_TEXT);
                        // printf("[INFO] Sent Huobi Pong: %s\n", pong_msg);

                        free(buf);
                    }
                    {
                        char price[32] = {0}, currency[32] = {0}, timestamp[64] = {0};
                        if (extract_numeric(decompressed, "\"close\":", price, sizeof(price))) {
                            extract_huobi_currency(decompressed, currency, sizeof(currency));
                            char ts_str[32] = {0};
                            if (extract_numeric(decompressed, "\"ts\":", ts_str, sizeof(ts_str))) {
                                convert_binance_timestamp(timestamp, sizeof(timestamp), ts_str);
                            } else {
                                get_timestamp(timestamp, sizeof(timestamp));
                            }
                            log_ticker_price(timestamp, "Huobi", currency, price);
                        }
                    }
                }
            }            
            else if (strcmp(protocol, "okx-websocket") == 0) {
                // printf("[TICKER][OKX] %.*s\n", (int)len, (char *)in);
                {
                    char price[32] = {0}, currency[32] = {0}, timestamp[64] = {0};
                    if (extract_price((char *)in, "\"last\":\"", price, sizeof(price)) &&
                        extract_price((char *)in, "\"instId\":\"", currency, sizeof(currency))) {
                        if (!extract_price((char *)in, "\"ts\":\"", timestamp, sizeof(timestamp)))
                            get_timestamp(timestamp, sizeof(timestamp));
                        log_ticker_price(timestamp, "OKX", currency, price);
                    }
                }
            }
            break;
            
        case LWS_CALLBACK_CLIENT_CLOSED:
            printf("[WARNING] %s WebSocket Connection Closed. Attempting Reconnect...\n", protocol);
            schedule_reconnect(protocol);
            break;
            
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            printf("[ERROR] %s WebSocket Connection Error! Attempting Reconnect...\n", protocol);
            schedule_reconnect(protocol);
            break;
            
        default:
            break;
    }
    
    return 0;
}

/* Define the protocols array for use in the context. */
struct lws_protocols protocols[] = {
    { "binance-websocket", callback_combined, 0, 4096, 0, 0, 0 },
    { "coinbase-websocket", callback_combined, 0, 4096, 0, 0, 0 },
    { "kraken-websocket", callback_combined, 0, 4096, 0, 0, 0 },
    { "bitfinex-websocket", callback_combined, 0, 4096, 0, 0, 0 },
    { "huobi-websocket", callback_combined, 0, 4096, 0, 0, 0 },
    { "okx-websocket", callback_combined, 0, 4096, 0, 0, 0 },
    { NULL, NULL, 0, 0, 0, 0, 0 }
};
