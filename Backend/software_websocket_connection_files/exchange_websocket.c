/*
 * Exchange WebSocket Handler
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
 * Updated: 3/11/2025
 */

#include "exchange_websocket.h"
#include "json_parser.h"
#include "utils.h"

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
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            if (strcmp(protocol, "binance-websocket") == 0) {
                printf("[INFO] Binance WebSocket Connection Established!\n");
                const char *subscribe_msg =
                    "{\"method\": \"SUBSCRIBE\", \"params\": ["
                    "\"btcusdt@ticker\", "
                    "\"ethusdt@ticker\", "
                    "\"adausdt@ticker\""
                    "], \"id\": 1}";
                size_t msg_len = strlen(subscribe_msg);
                unsigned char *buf = malloc(LWS_PRE + msg_len);
                if (!buf) {
                    printf("[ERROR] Memory allocation failed for Binance message\n");
                    return -1;
                }
                memcpy(buf + LWS_PRE, subscribe_msg, msg_len);
                int bytes_sent = lws_write(wsi, buf + LWS_PRE, msg_len, LWS_WRITE_TEXT);
                if (bytes_sent < 0)
                    printf("[ERROR] Failed to send Binance subscription message\n");
                else
                    printf("[INFO] Sent subscription message to Binance\n");
                free(buf);
            }
            else if (strcmp(protocol, "coinbase-websocket") == 0) {
                printf("[INFO] Coinbase WebSocket Connection Established!\n");
                const char *subscribe_msg =
                    "{\"type\": \"subscribe\", \"channels\": ["
                    "{ \"name\": \"ticker\", \"product_ids\": [\"BTC-USD\", \"ETH-USD\", \"ADA-USD\"] }"
                    "]}";
                size_t msg_len = strlen(subscribe_msg);
                unsigned char *buf = malloc(LWS_PRE + msg_len);
                if (!buf) {
                    printf("[ERROR] Memory allocation failed for Coinbase message\n");
                    return -1;
                }
                memcpy(buf + LWS_PRE, subscribe_msg, msg_len);
                int bytes_sent = lws_write(wsi, buf + LWS_PRE, msg_len, LWS_WRITE_TEXT);
                if (bytes_sent < 0)
                    printf("[ERROR] Failed to send Coinbase subscription message\n");
                else
                    printf("[INFO] Sent subscription message to Coinbase\n");
                free(buf);
            }
            else if (strcmp(protocol, "kraken-websocket") == 0) {
                printf("[INFO] Kraken WebSocket Connection Established!\n");
                const char *subscribe_msg =
                    "{\"event\": \"subscribe\", \"pair\": [\"XBT/USD\",\"ETH/USD\",\"ADA/USD\"], \"subscription\": {\"name\": \"ticker\"}}";
                size_t msg_len = strlen(subscribe_msg);
                unsigned char *buf = malloc(LWS_PRE + msg_len);
                if (!buf) {
                    printf("[ERROR] Memory allocation failed for Kraken message\n");
                    return -1;
                }
                memcpy(buf + LWS_PRE, subscribe_msg, msg_len);
                int bytes_sent = lws_write(wsi, buf + LWS_PRE, msg_len, LWS_WRITE_TEXT);
                if (bytes_sent < 0)
                    printf("[ERROR] Failed to send Kraken subscription message\n");
                else
                    printf("[INFO] Sent subscription message to Kraken\n");
                free(buf);
            }
            else if (strcmp(protocol, "bitfinex-websocket") == 0) {
                printf("[INFO] Bitfinex WebSocket Connection Established!\n");
                const char *subscribe_msg = "{\"event\": \"subscribe\", \"channel\": \"ticker\", \"symbol\": \"tBTCUSD\"}";
                size_t msg_len = strlen(subscribe_msg);
                unsigned char *buf = malloc(LWS_PRE + msg_len);
                if (!buf) {
                    printf("[ERROR] Memory allocation failed for Bitfinex message\n");
                    return -1;
                }
                memcpy(buf + LWS_PRE, subscribe_msg, msg_len);
                int bytes_sent = lws_write(wsi, buf + LWS_PRE, msg_len, LWS_WRITE_TEXT);
                if (bytes_sent < 0)
                    printf("[ERROR] Failed to send Bitfinex subscription message\n");
                else
                    printf("[INFO] Sent subscription message to Bitfinex\n");
                free(buf);
            }
            else if (strcmp(protocol, "huobi-websocket") == 0) {
                printf("[INFO] Huobi WebSocket Connection Established!\n");
                const char *subscribe_msg = "{\"sub\": \"market.btcusdt.ticker\", \"id\": \"huobi_ticker\"}";
                size_t msg_len = strlen(subscribe_msg);
                unsigned char *buf = malloc(LWS_PRE + msg_len);
                if (!buf) {
                    printf("[ERROR] Memory allocation failed for Huobi message\n");
                    return -1;
                }
                memcpy(buf + LWS_PRE, subscribe_msg, msg_len);
                int bytes_sent = lws_write(wsi, buf + LWS_PRE, msg_len, LWS_WRITE_TEXT);
                if (bytes_sent < 0)
                    printf("[ERROR] Failed to send Huobi subscription message\n");
                else
                    printf("[INFO] Sent subscription message to Huobi\n");
                free(buf);
            }
            else if (strcmp(protocol, "okx-websocket") == 0) {
                printf("[INFO] OKX WebSocket Connection Established!\n");
                const char *subscribe_msg = "{\"op\": \"subscribe\", \"args\": [{\"channel\": \"tickers\", \"instId\": \"BTC-USDT\"}]}";
                size_t msg_len = strlen(subscribe_msg);
                unsigned char *buf = malloc(LWS_PRE + msg_len);
                if (!buf) {
                    printf("[ERROR] Memory allocation failed for OKX message\n");
                    return -1;
                }
                memcpy(buf + LWS_PRE, subscribe_msg, msg_len);
                int bytes_sent = lws_write(wsi, buf + LWS_PRE, msg_len, LWS_WRITE_TEXT);
                if (bytes_sent < 0)
                    printf("[ERROR] Failed to send OKX subscription message\n");
                else
                    printf("[INFO] Sent subscription message to OKX\n");
                free(buf);
            }
            break;
            
        // (Comment in printf statements below to see full ticker outputs in terminal)
        case LWS_CALLBACK_CLIENT_RECEIVE:
            if (strcmp(protocol, "binance-websocket") == 0) {
                // printf("[DATA][Binance] %.*s\n", (int)len, (char *)in);
                {
                    char price[32] = {0}, currency[32] = {0}, time_ms[32] = {0}, timestamp[64] = {0};
                    if (extract_price((char *)in, "\"E\":", time_ms, sizeof(time_ms)) &&
                        extract_price((char *)in, "\"s\":\"", currency, sizeof(currency)) &&
                        extract_price((char *)in, "\"c\":\"", price, sizeof(price))) {
                        convert_binance_timestamp(timestamp, sizeof(timestamp), time_ms);
                        log_ticker_price(timestamp, "Binance", currency, price);
                    }
                }
            }
            else if (strcmp(protocol, "coinbase-websocket") == 0) {
                // printf("[DATA][Coinbase] %.*s\n", (int)len, (char *)in);
                {
                    char price[32] = {0}, currency[32] = {0}, timestamp[64] = {0};
                    if (extract_price((char *)in, "\"time\":\"", timestamp, sizeof(timestamp)) &&
                        extract_price((char *)in, "\"product_id\":\"", currency, sizeof(currency)) &&
                        extract_price((char *)in, "\"price\":\"", price, sizeof(price))) {
                            log_ticker_price(timestamp, "Coinbase", currency, price);
                    }
                }
            }
            else if (strcmp(protocol, "kraken-websocket") == 0) {
                if (strstr((char *)in, "\"event\":\"heartbeat\"")) {
                    return 0;
                }
                // printf("[DATA][Kraken] %.*s\n", (int)len, (char *)in);
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
                // printf("[DATA][Bitfinex] %.*s\n", (int)len, (char *)in);
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
                    // printf("[DATA][Huobi] %.*s\n", decompressed_len, decompressed);
            
                    // huobi requires a "ping" to recieve a "pong" in response
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
            else if (strcmp(protocol, "okx-websocket") == 0) {
                // printf("[DATA][OKX] %.*s\n", (int)len, (char *)in);
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
            printf("[INFO] %s WebSocket Connection Closed.\n", protocol);
            break;
            
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            printf("[ERROR] %s WebSocket Connection Error!\n", protocol);
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
