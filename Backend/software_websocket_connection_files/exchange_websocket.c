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
* Updated: 4/22/2025
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

#include <jansson.h>
#include <stdbool.h>

#include <bson.h>
#include <bson/bson.h>

char* build_subscription_from_file(const char *filename, const char *template_fmt) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "[ERROR] Could not open %s\n", filename);
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    rewind(fp);

    char *list = malloc(fsize + 1);
    if (!list) {
        fclose(fp);
        fprintf(stderr, "[ERROR] Memory allocation failed\n");
        return NULL;
    }

    fread(list, 1, fsize, fp);
    list[fsize] = '\0';
    fclose(fp);

    size_t msg_len = strlen(template_fmt) + strlen(list) + 128;
    char *subscribe_msg = malloc(msg_len);
    if (!subscribe_msg) {
        free(list);
        fprintf(stderr, "[ERROR] Memory allocation failed\n");
        return NULL;
    }

    snprintf(subscribe_msg, msg_len, template_fmt, list);
    free(list);
    return subscribe_msg;
}

char* build_huobi_subscription_from_file(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "[ERROR] Could not open %s\n", filename);
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    rewind(fp);

    char *symbols_raw = malloc(fsize + 1);
    if (!symbols_raw) {
        fclose(fp);
        fprintf(stderr, "[ERROR] Memory allocation failed\n");
        return NULL;
    }

    fread(symbols_raw, 1, fsize, fp);
    symbols_raw[fsize] = '\0';
    fclose(fp);

    // Allocate enough space for the full message
    size_t estimated_size = fsize * 3 + 256;
    char *subscribe_msg = malloc(estimated_size);
    if (!subscribe_msg) {
        free(symbols_raw);
        fprintf(stderr, "[ERROR] Memory allocation failed\n");
        return NULL;
    }

    strcpy(subscribe_msg, "[");  // begin array

    char *token = strtok(symbols_raw, "[\",\n ]");
    int first = 1;

    while (token) {
        if (!first) strcat(subscribe_msg, ",");
        first = 0;

        char entry[128];
        snprintf(entry, sizeof(entry),
            "{\"sub\": \"market.%s.ticker\", \"id\": \"huobi_%s\"}",
            token, token);
        strcat(subscribe_msg, entry);

        token = strtok(NULL, "[\",\n ]");
    }

    strcat(subscribe_msg, "]");  // end array

    free(symbols_raw);
    return subscribe_msg;
}

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
                    "{\"method\": \"SUBSCRIBE\", \"params\": [\"!ticker@arr\"], \"id\": 1}";
            }
            else if (strcmp(protocol, "coinbase-websocket") == 0) {
                subscribe_msg = build_subscription_from_file(
                    "coinbase_currency_ids.txt",
                    "{\"type\": \"subscribe\", \"channels\": [ { \"name\": \"ticker\", \"product_ids\": %s } ]}"
                );
                if (!subscribe_msg) return -1;
            }
            else if (strcmp(protocol, "kraken-websocket") == 0) {
                // subscribe_msg =
                //     "{\"event\": \"subscribe\", \"pair\": [\"XBT/USD\",\"ETH/USD\",\"ADA/USD\"], \"subscription\": {\"name\": \"ticker\"}}";
                subscribe_msg = build_subscription_from_file(
                    "kraken_currency_ids.txt",
                    "{\"event\": \"subscribe\", \"pair\": %s, \"subscription\": {\"name\": \"ticker\"}}"
                );
                if (!subscribe_msg) return -1;
            }
            else if (strcmp(protocol, "bitfinex-websocket") == 0) {
                subscribe_msg =
                    "{\"event\": \"subscribe\", \"channel\": \"ticker\", \"symbol\": \"tBTCUSD\"}";
            }
            else if (strcmp(protocol, "huobi-websocket") == 0) {
                // subscribe_msg =
                //     "{\"sub\": \"market.btcusdt.ticker\", \"id\": \"huobi_ticker\"}";

                FILE *fp = fopen("huobi_currency_ids.txt", "r");
                if (!fp) {
                    fprintf(stderr, "[ERROR] Could not open huobi_symbols.txt\n");
                    return -1;
                }

                fseek(fp, 0, SEEK_END);
                long fsize = ftell(fp);
                rewind(fp);

                char *file_buf = malloc(fsize + 1);
                if (!file_buf) {
                    fclose(fp);
                    fprintf(stderr, "[ERROR] Memory allocation failed\n");
                    return -1;
                }

                fread(file_buf, 1, fsize, fp);
                file_buf[fsize] = '\0';
                fclose(fp);

                // Parse symbols list like ["btcusdt", "ethusdt", ...]
                char *token = strtok(file_buf, "[\", \n]");
                while (token) {
                    char message[128];
                    snprintf(message, sizeof(message),
                            "{\"sub\": \"market.%s.ticker\", \"id\": \"huobi_%s\"}",
                            token, token);

                    printf("[DEBUG] Sending Huobi subscription: %s\n", message);

                    // Send the message using lws_write
                    size_t msg_len = strlen(message);
                    unsigned char *buf = malloc(LWS_PRE + msg_len);
                    if (!buf) {
                        fprintf(stderr, "[ERROR] Memory allocation for message buffer failed\n");
                        free(file_buf);
                        return -1;
                    }
                    memcpy(&buf[LWS_PRE], message, msg_len);
                    lws_write(wsi, &buf[LWS_PRE], msg_len, LWS_WRITE_TEXT);
                    free(buf);

                    token = strtok(NULL, "[\", \n]");
                }

                free(file_buf);
            }
            else if (strcmp(protocol, "okx-websocket") == 0) {
                // subscribe_msg =
                //     "{\"op\": \"subscribe\", \"args\": [{\"channel\": \"tickers\", \"instId\": \"BTC-USDT\"}]}";
                subscribe_msg = build_subscription_from_file(
                    "okx_currency_ids.txt",
                    "{\"op\": \"subscribe\", \"args\": %s}"
                );
                if (!subscribe_msg) return -1;
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
                    char trade_id[32] = {0}, market_maker[32] = {0};
                    if (extract_price((char *)in, "\"E\":", trade_time, sizeof(trade_time)) &&
                        extract_price((char *)in, "\"s\":\"", currency, sizeof(currency)) &&
                        extract_price((char *)in, "\"p\":\"", trade_price, sizeof(trade_price)) &&
                        extract_price((char *)in, "\"q\":\"", trade_size, sizeof(trade_size)) &&
                        extract_price((char *)in, "\"t\":\"", trade_id, sizeof(trade_id)) &&
                        extract_price((char *)in, "\"m\":\"", market_maker, sizeof(market_maker))) {

                        convert_binance_timestamp(timestamp, sizeof(timestamp), trade_time);
                        // printf("[TRADE] Binance | %s | Price: %s | Size: %s\n", currency, trade_price, trade_size);
                        log_trade_price(timestamp, "Binance", currency, trade_price, trade_size, trade_id, market_maker);
                    }
                }
                else {
                    TickerData binance_ticker = {0}; 
                    strncpy(binance_ticker.exchange, "Binance", MAX_EXCHANGE_NAME_LENGTH - 1);
                    binance_ticker.exchange[MAX_EXCHANGE_NAME_LENGTH - 1] = '\0'; 

                    if (extract_price(in, "\"E\":", binance_ticker.time_ms, sizeof(binance_ticker.time_ms)) &&
                        extract_price(in, "\"s\":\"", binance_ticker.currency, sizeof(binance_ticker.currency)) &&
                        extract_price(in, "\"c\":\"", binance_ticker.price, sizeof(binance_ticker.price))) {

                        extract_price(in, "\"b\":\"", binance_ticker.bid, sizeof(binance_ticker.bid));
                        extract_price(in, "\"B\":\"", binance_ticker.bid_qty, sizeof(binance_ticker.bid_qty));
                        extract_price(in, "\"a\":\"", binance_ticker.ask, sizeof(binance_ticker.ask));
                        extract_price(in, "\"A\":\"", binance_ticker.ask_qty, sizeof(binance_ticker.ask_qty));
                        extract_price(in, "\"o\":\"", binance_ticker.open_price, sizeof(binance_ticker.open_price));
                        extract_price(in, "\"h\":\"", binance_ticker.high_price, sizeof(binance_ticker.high_price));
                        extract_price(in, "\"l\":\"", binance_ticker.low_price, sizeof(binance_ticker.low_price));
                        extract_price(in, "\"v\":\"", binance_ticker.volume_24h, sizeof(binance_ticker.volume_24h));
                        extract_price(in, "\"q\":\"", binance_ticker.quote_volume, sizeof(binance_ticker.quote_volume));
                        extract_price(in, "\"t\":\"", binance_ticker.last_trade_time, sizeof(binance_ticker.last_trade_time)); 
                        extract_price(in, "\"p\":\"", binance_ticker.last_trade_price, sizeof(binance_ticker.last_trade_price)); 
                        extract_price(in, "\"C\":\"", binance_ticker.close_price, sizeof(binance_ticker.close_price));
                        extract_price(in, "\"S\":\"", binance_ticker.symbol, sizeof(binance_ticker.symbol));
                        
                        convert_binance_timestamp(binance_ticker.timestamp, sizeof(binance_ticker.timestamp), binance_ticker.time_ms);
    
                        log_ticker_price(&binance_ticker);
                        write_ticker_to_bson(&binance_ticker);

                    }
                }
            }
            else if (strcmp(protocol, "coinbase-websocket") == 0) {
                // printf("[DATA][Coinbase] %.*s\n", (int)len, (char *)in);
                if (strstr((char *)in, "\"type\":\"match\"") && !strstr((char *)in, "\"type\":\"last_match\"")) {
                    char trade_price[32] = {0}, trade_size[32] = {0}, timestamp[64] = {0}, currency[32] = {0};
                    char trade_id[32] = {0}, market_maker[32] = {0};
                    if (extract_price((char *)in, "\"time\":\"", timestamp, sizeof(timestamp)) &&
                        extract_price((char *)in, "\"product_id\":\"", currency, sizeof(currency)) &&
                        extract_price((char *)in, "\"price\":\"", trade_price, sizeof(trade_price)) &&
                        extract_price((char *)in, "\"size\":\"", trade_size, sizeof(trade_size))) {
                        // printf("[TRADE] Coinbase | %s | Price: %s | Size: %s\n", currency, trade_price, trade_size);
                        log_trade_price(timestamp, "Coinbase", currency, trade_price, trade_size, trade_id, market_maker);
                    }
                }
                else if (strstr((char *)in, "\"type\":\"ticker\"")) {
                    TickerData coinbase_ticker = {0};
                    strncpy(coinbase_ticker.exchange, "Coinbase", MAX_EXCHANGE_NAME_LENGTH - 1);
                    coinbase_ticker.exchange[MAX_EXCHANGE_NAME_LENGTH - 1] = '\0'; 

                    if (extract_price((char *)in, "\"time\":\"", coinbase_ticker.timestamp, sizeof(coinbase_ticker.timestamp)) &&
                        extract_price((char *)in, "\"product_id\":\"", coinbase_ticker.currency, sizeof(coinbase_ticker.currency)) &&
                        extract_price((char *)in, "\"price\":\"", coinbase_ticker.price, sizeof(coinbase_ticker.price))) {
                        printf("[TICKER] Coinbase | %s | Price: %s\n", coinbase_ticker.currency, coinbase_ticker.price);

                        extract_price((char *)in, "\"best_bid\":\"", coinbase_ticker.bid, sizeof(coinbase_ticker.bid));
                        extract_price((char *)in, "\"best_ask\":\"", coinbase_ticker.ask, sizeof(coinbase_ticker.ask));
                        extract_price((char *)in, "\"best_bid_size\":\"", coinbase_ticker.bid_qty, sizeof(coinbase_ticker.bid_qty));
                        extract_price((char *)in, "\"best_ask_size\":\"", coinbase_ticker.ask_qty, sizeof(coinbase_ticker.ask_qty));

                        extract_price((char *)in, "\"open_24h\":\"", coinbase_ticker.open_price, sizeof(coinbase_ticker.open_price));
                        extract_price((char *)in, "\"high_24h\":\"", coinbase_ticker.high_price, sizeof(coinbase_ticker.high_price));
                        extract_price((char *)in, "\"low_24h\":\"", coinbase_ticker.low_price, sizeof(coinbase_ticker.low_price));
                        extract_price((char *)in, "\"volume_24h\":\"", coinbase_ticker.volume_24h, sizeof(coinbase_ticker.volume_24h));
                        extract_price((char *)in, "\"volume_30d\":\"", coinbase_ticker.volume_30d, sizeof(coinbase_ticker.volume_30d));  
                        extract_price((char *)in, "\"trade_id\":", coinbase_ticker.trade_id, sizeof(coinbase_ticker.trade_id)); 
                        extract_price((char *)in, "\"last_size\":\"", coinbase_ticker.last_trade_size, sizeof(coinbase_ticker.last_trade_size));
                        log_ticker_price(&coinbase_ticker);
                        
                        write_ticker_to_bson(&coinbase_ticker);

                    }
                }
            }
            else if (strcmp(protocol, "kraken-websocket") == 0) {
                if (strstr((char *)in, "\"event\":\"heartbeat\"")) {
                    return 0;
                }
                // printf("[TICKER][Kraken] %.*s\n", (int)len, (char *)in);
                {
                    TickerData kraken_ticker = {0};
                    strncpy(kraken_ticker.exchange, "Kraken", MAX_EXCHANGE_NAME_LENGTH - 1);
                    kraken_ticker.exchange[MAX_EXCHANGE_NAME_LENGTH - 1] = '\0'; 
                    /* 
                        Kraken bid/ask information comes in the following format:
                        "b": ["bid price", "whole lot", "lot"]
                        "a": ["ask price", "whole lot", "lot"]
                    */
                   json_t *root, *obj, *b, *a, *c, *v, *p, *t, *l, *h, *o;
                   json_error_t err;
                   bool qty_found = true;

                    root = json_loads(in, 0, &err);
                    if (!root) {
                        qty_found = false;
                    }
                    else {
                        if (!json_is_array(root) || json_array_size(root) < 4) {
                            qty_found = false;
                        }
                        else { 
                            obj = json_array_get(root, 1);  
                            b = json_object_get(obj, "b");
                            a = json_object_get(obj, "a");
                            c = json_object_get(obj, "c");
                            v = json_object_get(obj, "v");
                            p = json_object_get(obj, "p");
                            t = json_object_get(obj, "t");
                            l = json_object_get(obj, "l");
                            h = json_object_get(obj, "h");
                            o = json_object_get(obj, "o");
                            
                            if  (json_is_string(json_array_get(b, 0)))
                                strncpy(kraken_ticker.bid,        json_string_value(json_array_get(b, 0)), sizeof(kraken_ticker.bid) - 1);
                            if  (json_is_string(json_array_get(a, 0)))
                                strncpy(kraken_ticker.ask,        json_string_value(json_array_get(a, 0)), sizeof(kraken_ticker.ask) - 1);
                            if  (json_is_string(json_array_get(b, 1)))
                                strncpy(kraken_ticker.bid_whole,  json_string_value(json_array_get(b, 1)), sizeof(kraken_ticker.bid_whole) - 1);
                            if  (json_is_string(json_array_get(b, 2)))
                                strncpy(kraken_ticker.bid_qty,    json_string_value(json_array_get(b, 2)), sizeof(kraken_ticker.bid_qty) - 1);
                            if  (json_is_string(json_array_get(a, 1)))
                                strncpy(kraken_ticker.ask_whole,  json_string_value(json_array_get(a, 1)), sizeof(kraken_ticker.ask_whole) - 1);
                            if  (json_is_string(json_array_get(a, 2)))
                                strncpy(kraken_ticker.ask_qty,    json_string_value(json_array_get(a, 2)), sizeof(kraken_ticker.ask_qty) - 1);
                            if  (json_is_string(json_array_get(c, 0)))
                                strncpy(kraken_ticker.price,      json_string_value(json_array_get(c, 0)), sizeof(kraken_ticker.price) - 1);                   
                            if  (json_is_string(json_array_get(c, 1)))
                                strncpy(kraken_ticker.last_vol,   json_string_value(json_array_get(c, 1)), sizeof(kraken_ticker.last_vol) - 1);
                            if  (json_is_string(json_array_get(v, 0)))
                                strncpy(kraken_ticker.vol_today,  json_string_value(json_array_get(v, 0)), sizeof(kraken_ticker.vol_today) - 1);
                            if  (json_is_string(json_array_get(v, 1)))
                                strncpy(kraken_ticker.volume_24h,    json_string_value(json_array_get(v, 1)), sizeof(kraken_ticker.volume_24h) - 1);
                            if  (json_is_string(json_array_get(p, 0)))
                                strncpy(kraken_ticker.vwap_today, json_string_value(json_array_get(p, 0)), sizeof(kraken_ticker.vwap_today) - 1);
                            if  (json_is_string(json_array_get(p, 1)))
                                strncpy(kraken_ticker.vwap_24h,   json_string_value(json_array_get(p, 1)), sizeof(kraken_ticker.vwap_24h) - 1);
                            if  (json_is_string(json_array_get(l, 0)))
                                strncpy(kraken_ticker.low_today,  json_string_value(json_array_get(l, 0)), sizeof(kraken_ticker.low_today) - 1);
                            if  (json_is_string(json_array_get(l, 1)))
                                strncpy(kraken_ticker.low_price,    json_string_value(json_array_get(l, 1)), sizeof(kraken_ticker.low_price) - 1);
                            if  (json_is_string(json_array_get(h, 0)))
                                strncpy(kraken_ticker.high_today, json_string_value(json_array_get(h, 0)), sizeof(kraken_ticker.high_today) - 1);
                            if  (json_is_string(json_array_get(h, 1)))
                                strncpy(kraken_ticker.high_price,   json_string_value(json_array_get(h, 1)), sizeof(kraken_ticker.high_price) - 1);
                            if  (json_is_string(json_array_get(o, "o")))
                                strncpy(kraken_ticker.open_today, json_string_value(json_object_get(o, "o")), sizeof(kraken_ticker.open_today) - 1);       
        
                        }
                    }
                    if (extract_price((char *)in, "\"c\":[\"", kraken_ticker.price, sizeof(kraken_ticker.price)) &&
                        qty_found ) {
                        const char *last_quote = strrchr((char *)in, '"');
                        if (last_quote) {
                            const char *start = last_quote - 1;
                            while (start > (char *)in && *start != '"') {
                                start--;
                            }
                            start++;
                            size_t len = last_quote - start;
                            if (len < sizeof(kraken_ticker.currency)) {
                                strncpy(kraken_ticker.currency, start, len);
                                kraken_ticker.currency[len] = '\0';
                            }
                        }
                        get_timestamp(kraken_ticker.timestamp, sizeof(kraken_ticker.timestamp));   
                        log_ticker_price(&kraken_ticker);
                        write_ticker_to_bson(&kraken_ticker);
                    }
                }
            }
            // else if (strcmp(protocol, "bitfinex-websocket") == 0) {
            //     if (strstr((char *)in, "\"hb\"")) {
            //         return 0;
            //     }
            //     // printf("[TICKER][Bitfinex] %.*s\n", (int)len, (char *)in);
            //     {
            //         char price[32] = {0}, timestamp[32] = {0};
            //         char bid[32] = {0}, ask[32] = {0}, bid_qty[32] = {0}, ask_qty[32] = {0};
            //         if (extract_bitfinex_price((char *)in, price, sizeof(price)) 
            //         /* &&
            //             extract_bitfinex_price((char *)in, "\"BID\":\"", bid, sizeof(bid)) &&
            //             extract_bitfinex_price((char *)in, "\"BID_SIZE\":\"", bid_qty, sizeof(bid_qty)) &&
            //             extract_bitfinex_price((char *)in, "\"ASK\":\"", ask, sizeof(ask)) &&
            //             extract_bitfinex_price((char *)in, "\"ASK_SIZE\":\"", ask_qty, sizeof(ask_qty))*/) {

            //             get_timestamp(timestamp, sizeof(timestamp));
            //             // log_ticker_price(timestamp, "Bitfinex", "tBTCUSD", price, bid, bid_qty, ask, ask_qty);
            //         }
            //     }
            // }
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
                    TickerData huobi_ticker = {0};
                    strncpy(huobi_ticker.exchange, "Huobi", MAX_EXCHANGE_NAME_LENGTH - 1);
                    huobi_ticker.exchange[MAX_EXCHANGE_NAME_LENGTH - 1] = '\0'; 

                    if (extract_numeric(decompressed, "\"close\":", huobi_ticker.price, sizeof(huobi_ticker.price)) &&
                        extract_huobi_currency(decompressed, huobi_ticker.currency, sizeof(huobi_ticker.currency))) {

                        extract_numeric(decompressed, "\"bid\":\"", huobi_ticker.bid, sizeof(huobi_ticker.bid));
                        extract_numeric(decompressed, "\"bidSize\":\"", huobi_ticker.bid_qty, sizeof(huobi_ticker.bid_qty));
                        extract_numeric(decompressed, "\"ask\":\"", huobi_ticker.ask, sizeof(huobi_ticker.ask));
                        extract_numeric(decompressed, "\"askSize\":\"", huobi_ticker.ask_qty, sizeof(huobi_ticker.ask_qty));

                        extract_numeric(decompressed, "\"open\":\"", huobi_ticker.open_price, sizeof(huobi_ticker.open_price));
                        extract_numeric(decompressed, "\"high\":\"", huobi_ticker.high_price, sizeof(huobi_ticker.high_price));
                        extract_numeric(decompressed, "\"low\":\"", huobi_ticker.low_price, sizeof(huobi_ticker.low_price));
                        extract_numeric(decompressed, "\"close\":\"", huobi_ticker.close_price, sizeof(huobi_ticker.close_price));

                        extract_numeric(decompressed, "\"amount\":\"", huobi_ticker.volume_24h, sizeof(huobi_ticker.volume_24h));
                        
                        char ts_str[32] = {0};
                        if (extract_numeric(decompressed, "\"ts\":", ts_str, sizeof(ts_str))) {
                            convert_binance_timestamp(huobi_ticker.timestamp, sizeof(huobi_ticker.timestamp), ts_str);
                        } else {
                            get_timestamp(huobi_ticker.timestamp, sizeof(huobi_ticker.timestamp));
                        }
                                                    
                        log_ticker_price(&huobi_ticker);
                        write_ticker_to_bson(&huobi_ticker);
                    }
                }
            }            
            else if (strcmp(protocol, "okx-websocket") == 0) {
                printf("[TICKER][OKX] %.*s\n", (int)len, (char *)in);

                TickerData okx_ticker = {0};
                strncpy(okx_ticker.exchange, "OKX", MAX_EXCHANGE_NAME_LENGTH - 1);
                okx_ticker.exchange[MAX_EXCHANGE_NAME_LENGTH - 1] = '\0'; 
                
                if (extract_price((char *)in, "\"last\":\"", okx_ticker.price, sizeof(okx_ticker.price)) &&
                    extract_price((char *)in, "\"instId\":\"", okx_ticker.currency, sizeof(okx_ticker.currency))) {

                    extract_price((char *)in, "\"bidPx\":\"", okx_ticker.bid, sizeof(okx_ticker.bid));
                    extract_price((char *)in, "\"bidSz\":\"", okx_ticker.bid_qty, sizeof(okx_ticker.bid_qty));
                    extract_price((char *)in, "\"askPx\":\"", okx_ticker.ask, sizeof(okx_ticker.ask));
                    extract_price((char *)in, "\"askSz\":\"", okx_ticker.ask_qty, sizeof(okx_ticker.ask_qty));

                    extract_price((char *)in, "\"open24h\":\"", okx_ticker.open_price, sizeof(okx_ticker.open_price));
                    extract_price((char *)in, "\"high24h\":\"", okx_ticker.high_price, sizeof(okx_ticker.high_price));
                    extract_price((char *)in, "\"low24h\":\"", okx_ticker.low_price, sizeof(okx_ticker.low_price));
                    extract_price((char *)in, "\"vol24h\":\"", okx_ticker.volume_24h, sizeof(okx_ticker.volume_24h));

                    if (!extract_price((char *)in, "\"ts\":\"", okx_ticker.timestamp, sizeof(okx_ticker.timestamp)))
                        get_timestamp(okx_ticker.timestamp, sizeof(okx_ticker.timestamp));
                    
                    log_ticker_price(&okx_ticker);
                    write_ticker_to_bson(&okx_ticker);
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





/* Write TickerData to a BSON file */
void write_ticker_to_bson(const TickerData *ticker) {
    time_t now = time(NULL);
    struct tm *tm = gmtime(&now);
    char filename[128];
    snprintf(filename, sizeof(filename), "%s_ticker_%04d%02d%02d.bson",
             ticker->exchange, tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);

    FILE *fp = fopen(filename, "ab");
    if (!fp) {
        printf("[ERROR] Failed to open BSON file %s: %s\n", filename, strerror(errno));
        return;
    }

    bson_t doc;
    bson_init(&doc);
    
    BSON_APPEND_UTF8(&doc, "exchange", ticker->exchange);
    BSON_APPEND_UTF8(&doc, "price", ticker->price);
    BSON_APPEND_UTF8(&doc, "currency", ticker->currency);
    BSON_APPEND_UTF8(&doc, "time_ms", ticker->time_ms);
    BSON_APPEND_UTF8(&doc, "timestamp", ticker->timestamp);
    BSON_APPEND_UTF8(&doc, "bid", ticker->bid);
    BSON_APPEND_UTF8(&doc, "ask", ticker->ask);
    BSON_APPEND_UTF8(&doc, "bid_qty", ticker->bid_qty);
    BSON_APPEND_UTF8(&doc, "ask_qty", ticker->ask_qty);
    BSON_APPEND_UTF8(&doc, "open_price", ticker->open_price);
    BSON_APPEND_UTF8(&doc, "high_price", ticker->high_price);
    BSON_APPEND_UTF8(&doc, "low_price", ticker->low_price);
    BSON_APPEND_UTF8(&doc, "close_price", ticker->close_price);
    BSON_APPEND_UTF8(&doc, "volume_24h", ticker->volume_24h);
    BSON_APPEND_UTF8(&doc, "volume_30d", ticker->volume_30d);
    BSON_APPEND_UTF8(&doc, "quote_volume", ticker->quote_volume);
    BSON_APPEND_UTF8(&doc, "symbol", ticker->symbol);
    BSON_APPEND_UTF8(&doc, "last_trade_time", ticker->last_trade_time);
    BSON_APPEND_UTF8(&doc, "last_trade_price", ticker->last_trade_price);
    BSON_APPEND_UTF8(&doc, "last_trade_size", ticker->last_trade_size);
    BSON_APPEND_UTF8(&doc, "trade_id", ticker->trade_id);
    BSON_APPEND_UTF8(&doc, "sequence", ticker->sequence);
    
    // relatively new fields
    BSON_APPEND_UTF8(&doc, "bid_whole", ticker->bid_whole);
    BSON_APPEND_UTF8(&doc, "ask_whole", ticker->ask_whole);
    BSON_APPEND_UTF8(&doc, "last_vol", ticker->last_vol);
    BSON_APPEND_UTF8(&doc, "vol_today", ticker->vol_today);
    BSON_APPEND_UTF8(&doc, "vwap_today", ticker->vwap_today);
    BSON_APPEND_UTF8(&doc, "vwap_24h", ticker->vwap_24h);
    BSON_APPEND_UTF8(&doc, "low_today", ticker->low_today);
    BSON_APPEND_UTF8(&doc, "high_today", ticker->high_today);
    BSON_APPEND_UTF8(&doc, "open_today", ticker->open_today);


    const uint8_t *data = bson_get_data(&doc);
    if (fwrite(data, 1, doc.len, fp) != doc.len) {
        printf("[ERROR] Failed to write to BSON file %s\n", filename);
    } else {
        printf("[INFO] Wrote TickerData to %s\n", filename);
    }

    bson_destroy(&doc);
    fclose(fp);
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
