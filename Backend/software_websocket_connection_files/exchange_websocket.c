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
                
                
                
                subscribe_msg =
                    "{\"type\": \"subscribe\", \"channels\": ["
                    "{ \"name\": \"ticker\", \"product_ids\": [\"BTC-USD\", \"ETH-USD\", \"ADA-USD\"] },"
                    "{ \"name\": \"matches\", \"product_ids\": [\"BTC-USD\", \"ETH-USD\", \"ADA-USD\"] }"
                    "]}";
                    // "{\"type\": \"subscribe\", \"channels\": ["
                    // "{ \"name\": \"ticker\" },"
                    // "{ \"name\": \"matches\" }"
                    // "]}";
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
                    char price[32] = {0}, currency[32] = {0}, timestamp[32] = {0};
                    char bid[32] = {0}, ask[32] = {0};
                    /* 
                        Kraken bid/ask information comes in the following format:
                        "b": ["bid price", "whole lot", "lot"]
                        "a": ["ask price", "whole lot", "lot"]
                    */
                   json_t *b, *a, *obj, *root;
                   json_error_t err;
                   const char *bid_qty, *ask_qty;
                   json_t *bid_qty_json, *ask_qty_json;
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
                            bid_qty_json = json_array_get(b, 2);
                            ask_qty_json = json_array_get(a, 2);
                            if (!json_is_string(bid_qty_json) || !json_is_string(ask_qty_json)) {
                                qty_found = false;
                            }
                            else {
                                bid_qty = json_string_value(bid_qty_json);
                                ask_qty = json_string_value(ask_qty_json);
                            }
                        }
                    }
                    if (extract_price((char *)in, "\"c\":[\"", price, sizeof(price)) &&
                        extract_price((char *)in, "\"b\":[\"", bid, sizeof(bid)) &&
                        extract_price((char *)in, "\"a\":[\"", ask, sizeof(ask)) &&
                        qty_found ) {
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
                        // printf("[TRADE] Kraken | %s | Price: %s | Bid Price: %s| Bid Quantity: %s | Ask Price: %s | Ask Quantity: %s\n", currency, price, bid, bid_qty, ask, ask_qty);                       
                        // log_ticker_price(timestamp, "Kraken", currency, price, bid, bid_qty, ask, ask_qty);
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
                    char bid[32] = {0}, ask[32] = {0}, bid_qty[32] = {0}, ask_qty[32] = {0};
                    if (extract_bitfinex_price((char *)in, price, sizeof(price)) 
                    /* &&
                        extract_bitfinex_price((char *)in, "\"BID\":\"", bid, sizeof(bid)) &&
                        extract_bitfinex_price((char *)in, "\"BID_SIZE\":\"", bid_qty, sizeof(bid_qty)) &&
                        extract_bitfinex_price((char *)in, "\"ASK\":\"", ask, sizeof(ask)) &&
                        extract_bitfinex_price((char *)in, "\"ASK_SIZE\":\"", ask_qty, sizeof(ask_qty))*/) {

                        get_timestamp(timestamp, sizeof(timestamp));
                        // log_ticker_price(timestamp, "Bitfinex", "tBTCUSD", price, bid, bid_qty, ask, ask_qty);
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
                        char bid[32] = {0}, ask[32] = {0}, bid_qty[32] = {0}, ask_qty[32] = {0};
                        if (extract_numeric(decompressed, "\"close\":", price, sizeof(price)) &&
                            extract_numeric(decompressed, "\"bid\":\"", bid, sizeof(bid)) &&
                            extract_numeric(decompressed, "\"bidSize\":\"", bid_qty, sizeof(bid_qty)) &&
                            extract_numeric(decompressed, "\"ask\":\"", ask, sizeof(ask)) &&
                            extract_numeric(decompressed, "\"askSize\":\"", ask_qty, sizeof(ask_qty))) {

                            extract_huobi_currency(decompressed, currency, sizeof(currency));
                            char ts_str[32] = {0};
                            if (extract_numeric(decompressed, "\"ts\":", ts_str, sizeof(ts_str))) {
                                convert_binance_timestamp(timestamp, sizeof(timestamp), ts_str);
                            } else {
                                get_timestamp(timestamp, sizeof(timestamp));
                            }
                                                        
                            // log_ticker_price(timestamp, "Huobi", currency, price, bid, bid_qty, ask, ask_qty);
                        }
                    }
                }
            }            
            else if (strcmp(protocol, "okx-websocket") == 0) {
                // printf("[TICKER][OKX] %.*s\n", (int)len, (char *)in);
                {
                    char price[32] = {0}, currency[32] = {0}, timestamp[64] = {0};
                    char bid[32] = {0}, ask[32] = {0}, bid_qty[32] = {0}, ask_qty[32] = {0};
                    if (extract_price((char *)in, "\"last\":\"", price, sizeof(price)) &&
                        extract_price((char *)in, "\"instId\":\"", currency, sizeof(currency)) &&
                        extract_price((char *)in, "\"bidPx\":\"", bid, sizeof(bid)) &&
                        extract_price((char *)in, "\"bidSz\":\"", bid_qty, sizeof(bid_qty)) &&
                        extract_price((char *)in, "\"askPx\":\"", ask, sizeof(ask)) &&
                        extract_price((char *)in, "\"askSz\":\"", ask_qty, sizeof(ask_qty))) {

                        if (!extract_price((char *)in, "\"ts\":\"", timestamp, sizeof(timestamp)))
                            get_timestamp(timestamp, sizeof(timestamp));
                        // log_ticker_price(timestamp, "OKX", currency, price, bid, bid_qty, ask, ask_qty);
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

    // BSON_APPEND_UTF8(&doc, "test", ticker);

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
