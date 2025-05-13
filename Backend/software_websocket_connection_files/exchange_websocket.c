/*
 * Exchange WebSocket
 * 
 * Handles real-time WebSocket connections to multiple cryptocurrency exchanges
 * for both trade and ticker data, with custom parsing and structured output.
 * 
 * Features:
 *  - Unified callback (`callback_combined`) for all supported exchanges.
 *  - Exchange-specific message handling for Binance, Coinbase, Kraken, OKX, Huobi, and Bitfinex.
 *  - Parses JSON (including nested arrays) and decompresses gzip payloads.
 *  - Logs parsed trades and tickers to JSON output and BSON files for storage.
 *  - Supports chunked subscription logic and multi-channel stream merging.
 *  - Robust reconnection and heartbeat handling across all protocols.
 * 
 * Dependencies:
 *  - libwebsockets: WebSocket communication.
 *  - jansson: JSON parsing.
 *  - libbson: BSON serialization.
 *  - zlib: GZIP decompression for Huobi.
 *  - Standard C libraries (stdio, string, stdlib, errno).
 * 
 * Usage:
 *  - Entry point for WebSocket activity in `main.c`, registered via `protocols[]`.
 *  - Requires ID lists in `currency_text_files/` for building subscriptions.
 * 
 * Created: 3/7/2025
 * Updated: 5/11/2025
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
#include <libwebsockets.h>

#include <bson.h>
#include <bson/bson.h>


/* Build a subscription message by reading a single file and formatting it into a template string */
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

    size_t msg_len = strlen(template_fmt) + (2 * strlen(list)) + 128;
    char *subscribe_msg = malloc(msg_len);
    if (!subscribe_msg) {
        free(list);
        fprintf(stderr, "[ERROR] Memory allocation failed\n");
        return NULL;
    }

    snprintf(subscribe_msg, msg_len, template_fmt, list, list);
    free(list);
    return subscribe_msg;
}

/* Send chunked subscription messages to Kraken using pairs from a JSON file and LWS connection */
int build_kraken_subscription_from_file(struct lws *wsi, const char *filename, size_t chunk_size) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "[ERROR] Could not open %s\n", filename);
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    rewind(fp);

    char *file_data = malloc(fsize + 1);
    if (!file_data) {
        fclose(fp);
        fprintf(stderr, "[ERROR] Memory allocation failed\n");
        return -1;
    }

    fread(file_data, 1, fsize, fp);
    file_data[fsize] = '\0';
    fclose(fp);

    json_error_t error;
    json_t *pair_array = json_loads(file_data, 0, &error);
    free(file_data);

    if (!pair_array || !json_is_array(pair_array)) {
        fprintf(stderr, "[ERROR] Failed to parse JSON array: %s\n", error.text);
        if (pair_array) json_decref(pair_array);
        return -1;
    }

    size_t total = json_array_size(pair_array);

    for (size_t i = 0; i < total; i += chunk_size) {
        json_t *chunk = json_array();
        size_t end = (i + chunk_size > total) ? total : i + chunk_size;

        for (size_t j = i; j < end; j++) {
            json_array_append(chunk, json_array_get(pair_array, j));
        }

        char *pair_list_str = json_dumps(chunk, JSON_ENSURE_ASCII);
        json_decref(chunk);
        if (!pair_list_str) {
            json_decref(pair_array);
            fprintf(stderr, "[ERROR] Failed to serialize chunk JSON\n");
            return -1;
        }

        const char *channels[] = { "ticker", "trade" };
        for (int c = 0; c < 2; c++) {
            size_t msg_size = strlen(pair_list_str) + 128;
            char *subscribe_msg = malloc(msg_size);
            if (!subscribe_msg) {
                free(pair_list_str);
                json_decref(pair_array);
                fprintf(stderr, "[ERROR] Memory allocation failed for subscribe_msg\n");
                return -1;
            }

            snprintf(subscribe_msg, msg_size,
                "{\"event\": \"subscribe\", \"pair\": %s, \"subscription\": {\"name\": \"%s\"}}",
                pair_list_str, channels[c]);

            unsigned char buf[LWS_PRE + 2048];
            size_t len = strlen(subscribe_msg);
            memcpy(&buf[LWS_PRE], subscribe_msg, len);

            int n = lws_write(wsi, &buf[LWS_PRE], len, LWS_WRITE_TEXT);
            if (n < 0) {
                fprintf(stderr, "[ERROR] Failed to send %s subscription\n", channels[c]);
                free(subscribe_msg);
                free(pair_list_str);
                json_decref(pair_array);
                return -1;
            }

            // printf("[DEBUG] Sent Kraken %s chunk: %s\n", channels[c], subscribe_msg);
            free(subscribe_msg);
        }

        free(pair_list_str);
    }

    json_decref(pair_array);
    return 0;
}

/* Build a Huobi subscription message by parsing a plain text file of symbols into JSON requests */
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

    strcpy(subscribe_msg, "[");

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

    strcat(subscribe_msg, "]");

    free(symbols_raw);
    return subscribe_msg;
}

/* Build a subscription message using data from two JSON files */
char* build_subscription_from_two_files(const char *file1, const char *file2, const char *template_fmt) {
    FILE *fp1 = fopen(file1, "r");
    FILE *fp2 = fopen(file2, "r");
    if (!fp1 || !fp2) {
        fprintf(stderr, "[ERROR] Could not open %s or %s\n", file1, file2);
        if (fp1) fclose(fp1);
        if (fp2) fclose(fp2);
        return NULL;
    }

    // Read entire content of both files
    fseek(fp1, 0, SEEK_END);
    long size1 = ftell(fp1);
    rewind(fp1);
    char *data1 = malloc(size1 + 1);
    fread(data1, 1, size1, fp1);
    data1[size1] = '\0';
    fclose(fp1);

    fseek(fp2, 0, SEEK_END);
    long size2 = ftell(fp2);
    rewind(fp2);
    char *data2 = malloc(size2 + 1);
    fread(data2, 1, size2, fp2);
    data2[size2] = '\0';
    fclose(fp2);

    // Allocate space for the final combined args array
    char *combined = malloc(size1 + size2 + 128); // extra buffer room
    if (!combined) {
        fprintf(stderr, "[ERROR] Memory allocation failed\n");
        free(data1);
        free(data2);
        return NULL;
    }
    strcpy(combined, "[");

    // Helper macro for appending full JSON object lists from each file
    #define APPEND_ENTRIES(data)                             \
        do {                                                 \
            char *p = data + 1;                              \
            while (*p && *p != ']') {                        \
                strncat(combined, p, 1);                     \
                p++;                                         \
            }                                                \
        } while (0)

    APPEND_ENTRIES(data1);
    if (combined[strlen(combined) - 1] != '[') strcat(combined, ",");

    APPEND_ENTRIES(data2);
    strcat(combined, "]");

    #undef APPEND_ENTRIES

    // Build final subscription message
    size_t msg_len = strlen(template_fmt) + strlen(combined) + 64;
    char *subscribe_msg = malloc(msg_len);
    if (!subscribe_msg) {
        fprintf(stderr, "[ERROR] Memory allocation for final message failed\n");
        free(data1);
        free(data2);
        free(combined);
        return NULL;
    }

    snprintf(subscribe_msg, msg_len, template_fmt, combined);

    free(data1);
    free(data2);
    free(combined);

    return subscribe_msg;
}


/* Build a Binance combined stream subscription message from a file of symbols and format for WebSocket */
char *build_binance_combined_subscription(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "[ERROR] Could not open %s\n", filename);
        return NULL;
    }

    // Allocate dynamic buffer
    size_t capacity = 8192;
    char *params = malloc(capacity);
    if (!params) {
        fclose(fp);
        return NULL;
    }
    params[0] = '\0';

    int first = 1;
    char line[64];
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = 0;

        if (strlen(line) == 0)
            continue;

        // Prepare entry string
        char entry[160];
        snprintf(entry, sizeof(entry), "\"%s@ticker\",\"%s@trade\"", line, line);

        // Expand buffer if needed
        size_t needed = strlen(params) + strlen(entry) + 2;
        if (needed >= capacity) {
            capacity *= 2;
            char *new_params = realloc(params, capacity);
            if (!new_params) {
                free(params);
                fclose(fp);
                return NULL;
            }
            params = new_params;
        }

        if (!first)
            strcat(params, ",");

        strcat(params, entry);
        first = 0;
    }
    fclose(fp);

    // Final message JSON
    size_t final_len = strlen(params) + 64;
    char *subscribe_msg = malloc(final_len);
    if (!subscribe_msg) {
        free(params);
        return NULL;
    }

    snprintf(subscribe_msg, final_len, "{\"method\": \"SUBSCRIBE\", \"params\": [%s], \"id\": 1}", params);
    free(params);
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
            char *subscribe_msg = NULL;
            if (strncmp(protocol, "binance-websocket-", 18) == 0) {
                int chunk_index = atoi(protocol + 19);
            
                char filename[64];
                snprintf(filename, sizeof(filename), "currency_text_files/binance_currency_chunk_trades_%d.txt", chunk_index);
            
                subscribe_msg = build_binance_combined_subscription(filename);
            }            
            else if (strcmp(protocol, "coinbase-websocket") == 0) {
                subscribe_msg = build_subscription_from_file(
                    "currency_text_files/coinbase_currency_ids.txt",
                        "{\"type\": \"subscribe\", \"channels\": ["
                        "{ \"name\": \"ticker\", \"product_ids\": %s },"
                        "{ \"name\": \"matches\", \"product_ids\": %s } ]}"
                    );

                if (!subscribe_msg) return -1;
            }
            else if (strcmp(protocol, "kraken-websocket") == 0) {
                usleep(200000);
                if (build_kraken_subscription_from_file(wsi, "currency_text_files/kraken_currency_ids.txt", 100) != 0) {
                    return -1;
                }
            }
            else if (strcmp(protocol, "bitfinex-websocket") == 0) {
                subscribe_msg =
                    "{\"event\": \"subscribe\", \"channel\": \"ticker\", \"symbol\": \"tBTCUSD\"}";
            }
            else if ((strncmp(protocol, "huobi-websocket-", 16) == 0)) {
                // printf("[DEBUG] Protocol name is: %s\n", protocol);
                int chunk_index = atoi(protocol + 17);
                char filename[64];
                snprintf(filename, sizeof(filename), "currency_text_files/huobi_currency_chunk_%d.txt", chunk_index);
            
                FILE *fp = fopen(filename, "r");
                if (!fp) {
                    fprintf(stderr, "[ERROR] Could not open %s\n", filename);
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
            
                char *token = strtok(file_buf, "[\", \n]");
                while (token) {
                    char ticker_msg[128];
                    char trade_msg[128];
                
                    snprintf(ticker_msg, sizeof(ticker_msg),
                             "{\"sub\": \"market.%s.ticker\", \"id\": \"huobi_%s_ticker\"}",
                             token, token);
                    snprintf(trade_msg, sizeof(trade_msg),
                             "{\"sub\": \"market.%s.trade.detail\", \"id\": \"huobi_%s_trade\"}",
                             token, token);
                
                    for (int i = 0; i < 2; i++) {
                        const char *msg = (i == 0) ? ticker_msg : trade_msg;
                        size_t msg_len = strlen(msg);
                        unsigned char *buf = malloc(LWS_PRE + msg_len);
                        if (!buf) {
                            fprintf(stderr, "[ERROR] Memory allocation failed for Huobi message\n");
                            free(file_buf);
                            return -1;
                        }
                        memcpy(buf + LWS_PRE, msg, msg_len);
                        lws_write(wsi, buf + LWS_PRE, msg_len, LWS_WRITE_TEXT);
                        free(buf);
                    }
                
                    token = strtok(NULL, "[\", \n]");
                }                
            
                free(file_buf);
            }            
            else if (strncmp(protocol, "okx-websocket-", 14) == 0) {
                int chunk_index = atoi(protocol + 15);
            
                char file1[64], file2[64];
                snprintf(file1, sizeof(file1), "currency_text_files/okx_currency_chunk_%d.txt", chunk_index);
                snprintf(file2, sizeof(file2), "currency_text_files/okx_currency_chunk_trades_%d.txt", chunk_index);
            
                subscribe_msg = build_subscription_from_two_files(
                    file1,
                    file2,
                    "{\"op\": \"subscribe\", \"args\": %s}"
                );
                if (!subscribe_msg) return -1;
                // printf("[DEBUG] OKX Subscription Message:\n%s\n", subscribe_msg);
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
        case LWS_CALLBACK_CLIENT_RECEIVE: {
            int idx = get_exchange_index(protocol);
            if (idx != -1) {
                last_message_time[idx] = time(NULL);
            }

            if (strncmp(protocol, "binance-websocket", 17) == 0) {
                // printf("[DATA][Binance] %.*s\n", (int)len, (char *)in);
                char *msg = malloc(len + 1);
                memcpy(msg, in, len);
                msg[len] = '\0';
                if (strstr(msg, "\"e\":\"trade\"")) {
                    TradeData binance_trade = {0}; 
                    strncpy(binance_trade.exchange, "Binance", sizeof(binance_trade.exchange) - 1);

                    char trade_time[32] = {0};
                    if (extract_order_data(msg, "\"E\":", trade_time, sizeof(trade_time)) && 
                        extract_order_data(msg, "\"s\":\"", binance_trade.currency, sizeof(binance_trade.currency)) && 
                        extract_order_data(msg, "\"p\":\"", binance_trade.price, sizeof(binance_trade.price)) && 
                        extract_order_data(msg, "\"q\":\"", binance_trade.size, sizeof(binance_trade.size)) && 
                        extract_order_data(msg, "\"t\":", binance_trade.trade_id, sizeof(binance_trade.trade_id)) && 
                        extract_order_data(msg, "\"m\":", binance_trade.market_maker, sizeof(binance_trade.market_maker))) {

                        convert_binance_timestamp(binance_trade.timestamp, sizeof(binance_trade.timestamp), trade_time);
                        log_trade_price(binance_trade.timestamp, binance_trade.exchange, binance_trade.currency,
                                        binance_trade.price, binance_trade.size, binance_trade.trade_id, binance_trade.market_maker);
                        write_trade_to_bson(&binance_trade);
                        // printf("[TRADE] %s | %s | Price: %s | Size: %s | ID: %s | MM: %s\n", binance_trade.exchange, binance_trade.currency, binance_trade.price, binance_trade.size, binance_trade.trade_id, binance_trade.market_maker);
                    }
                } 
                else {
                    // printf("[DEBUG] '\"e\":\"trade\"' not found in input\n");
                    TickerData binance_ticker = {0}; 
                    strncpy(binance_ticker.exchange, "Binance", MAX_EXCHANGE_NAME_LENGTH - 1);
                    binance_ticker.exchange[MAX_EXCHANGE_NAME_LENGTH - 1] = '\0'; 

                    if (extract_order_data(in, "\"E\":", binance_ticker.time_ms, sizeof(binance_ticker.time_ms)) &&
                        extract_order_data(in, "\"s\":\"", binance_ticker.currency, sizeof(binance_ticker.currency)) &&
                        extract_order_data(in, "\"c\":\"", binance_ticker.price, sizeof(binance_ticker.price))) {

                        extract_order_data(in, "\"b\":\"", binance_ticker.bid, sizeof(binance_ticker.bid));
                        extract_order_data(in, "\"B\":\"", binance_ticker.bid_qty, sizeof(binance_ticker.bid_qty));
                        extract_order_data(in, "\"a\":\"", binance_ticker.ask, sizeof(binance_ticker.ask));
                        extract_order_data(in, "\"A\":\"", binance_ticker.ask_qty, sizeof(binance_ticker.ask_qty));
                        extract_order_data(in, "\"o\":\"", binance_ticker.open_price, sizeof(binance_ticker.open_price));
                        extract_order_data(in, "\"h\":\"", binance_ticker.high_price, sizeof(binance_ticker.high_price));
                        extract_order_data(in, "\"l\":\"", binance_ticker.low_price, sizeof(binance_ticker.low_price));
                        extract_order_data(in, "\"v\":\"", binance_ticker.volume_24h, sizeof(binance_ticker.volume_24h));
                        extract_order_data(in, "\"q\":\"", binance_ticker.quote_volume, sizeof(binance_ticker.quote_volume));
                        extract_order_data(in, "\"t\":\"", binance_ticker.last_trade_time, sizeof(binance_ticker.last_trade_time)); 
                        extract_order_data(in, "\"p\":\"", binance_ticker.last_trade_price, sizeof(binance_ticker.last_trade_price)); 
                        extract_order_data(in, "\"C\":\"", binance_ticker.close_price, sizeof(binance_ticker.close_price));
                        extract_order_data(in, "\"S\":\"", binance_ticker.symbol, sizeof(binance_ticker.symbol));
                        
                        convert_binance_timestamp(binance_ticker.timestamp, sizeof(binance_ticker.timestamp), binance_ticker.time_ms);
    
                        log_ticker_price(&binance_ticker);
                        write_ticker_to_bson(&binance_ticker);

                    }
                }
                free(msg);
            }
            else if (strcmp(protocol, "coinbase-websocket") == 0) {
                // printf("[DATA][Coinbase] %.*s\n", (int)len, (char *)in);
                if (strstr((char *)in, "\"type\":\"match\"") && !strstr((char *)in, "\"type\":\"last_match\"")) {
                    TradeData coinbase_trade = {0};
                    strncpy(coinbase_trade.exchange, "Coinbase", sizeof(coinbase_trade.exchange) - 1);

                    if (extract_order_data((char *)in, "\"time\":\"", coinbase_trade.timestamp, sizeof(coinbase_trade.timestamp)) &&
                        extract_order_data((char *)in, "\"product_id\":\"", coinbase_trade.currency, sizeof(coinbase_trade.currency)) &&
                        extract_order_data((char *)in, "\"price\":\"", coinbase_trade.price, sizeof(coinbase_trade.price)) &&
                        extract_order_data((char *)in, "\"size\":\"", coinbase_trade.size, sizeof(coinbase_trade.size))) {

                        extract_order_data((char *)in, "\"trade_id\":", coinbase_trade.trade_id, sizeof(coinbase_trade.trade_id));

                        log_trade_price(coinbase_trade.timestamp, coinbase_trade.exchange, coinbase_trade.currency,
                                        coinbase_trade.price, coinbase_trade.size, coinbase_trade.trade_id, coinbase_trade.market_maker);

                        write_trade_to_bson(&coinbase_trade);
                        // printf("[TRADE] %s | %s | Price: %s | Size: %s | ID: %s\n", coinbase_trade.exchange, coinbase_trade.currency, coinbase_trade.price, coinbase_trade.size, coinbase_trade.trade_id);
                    }
                }
                else if (strstr((char *)in, "\"type\":\"ticker\"")) {
                    TickerData coinbase_ticker = {0};
                    strncpy(coinbase_ticker.exchange, "Coinbase", MAX_EXCHANGE_NAME_LENGTH - 1);
                    coinbase_ticker.exchange[MAX_EXCHANGE_NAME_LENGTH - 1] = '\0'; 

                    if (extract_order_data((char *)in, "\"time\":\"", coinbase_ticker.timestamp, sizeof(coinbase_ticker.timestamp)) &&
                        extract_order_data((char *)in, "\"product_id\":\"", coinbase_ticker.currency, sizeof(coinbase_ticker.currency)) &&
                        extract_order_data((char *)in, "\"price\":\"", coinbase_ticker.price, sizeof(coinbase_ticker.price))) {
                        // printf("[TICKER] Coinbase | %s | Price: %s\n", coinbase_ticker.currency, coinbase_ticker.price);

                        extract_order_data((char *)in, "\"best_bid\":\"", coinbase_ticker.bid, sizeof(coinbase_ticker.bid));
                        extract_order_data((char *)in, "\"best_ask\":\"", coinbase_ticker.ask, sizeof(coinbase_ticker.ask));
                        extract_order_data((char *)in, "\"best_bid_size\":\"", coinbase_ticker.bid_qty, sizeof(coinbase_ticker.bid_qty));
                        extract_order_data((char *)in, "\"best_ask_size\":\"", coinbase_ticker.ask_qty, sizeof(coinbase_ticker.ask_qty));

                        extract_order_data((char *)in, "\"open_24h\":\"", coinbase_ticker.open_price, sizeof(coinbase_ticker.open_price));
                        extract_order_data((char *)in, "\"high_24h\":\"", coinbase_ticker.high_price, sizeof(coinbase_ticker.high_price));
                        extract_order_data((char *)in, "\"low_24h\":\"", coinbase_ticker.low_price, sizeof(coinbase_ticker.low_price));
                        extract_order_data((char *)in, "\"volume_24h\":\"", coinbase_ticker.volume_24h, sizeof(coinbase_ticker.volume_24h));
                        extract_order_data((char *)in, "\"volume_30d\":\"", coinbase_ticker.volume_30d, sizeof(coinbase_ticker.volume_30d));  
                        extract_order_data((char *)in, "\"trade_id\":", coinbase_ticker.trade_id, sizeof(coinbase_ticker.trade_id)); 
                        extract_order_data((char *)in, "\"last_size\":\"", coinbase_ticker.last_trade_size, sizeof(coinbase_ticker.last_trade_size));
                        log_ticker_price(&coinbase_ticker);
                        
                        write_ticker_to_bson(&coinbase_ticker);

                    }
                }
            }
            else if (strcmp(protocol, "kraken-websocket") == 0) {
                // Handle Kraken trade messages
                if (strstr((char *)in, "\"trade\"")) {
                    json_error_t err;
                    json_t *root = json_loads((char *)in, 0, &err);
                    if (root && json_is_array(root) && json_array_size(root) >= 4) {
                        json_t *trades = json_array_get(root, 1);  // array of trades
                        json_t *meta = json_array_get(root, json_array_size(root) - 1);
                        const char *pair = json_string_value(meta);
                        if (json_is_array(trades)) {
                            for (size_t i = 0; i < json_array_size(trades); i++) {
                                json_t *t = json_array_get(trades, i);
                                if (json_is_array(t) && json_array_size(t) >= 3) {
                                    TradeData kraken_trade = {0};
                                    strncpy(kraken_trade.exchange, "Kraken", sizeof(kraken_trade.exchange) - 1);
                                    if (pair)
                                        strncpy(kraken_trade.currency, pair, sizeof(kraken_trade.currency) - 1);
                                    
                                    const char *price = json_string_value(json_array_get(t, 0));
                                    const char *size = json_string_value(json_array_get(t, 1));
                                    const char *time = json_string_value(json_array_get(t, 2));

                                    if (price) strncpy(kraken_trade.price, price, sizeof(kraken_trade.price) - 1);
                                    if (size) strncpy(kraken_trade.size, size, sizeof(kraken_trade.size) - 1);
                                    if (time) strncpy(kraken_trade.timestamp, time, sizeof(kraken_trade.timestamp) - 1);
                                    else get_timestamp(kraken_trade.timestamp, sizeof(kraken_trade.timestamp));

                                    log_trade_price(kraken_trade.timestamp, kraken_trade.exchange, kraken_trade.currency,
                                                    kraken_trade.price, kraken_trade.size, kraken_trade.trade_id, kraken_trade.market_maker);
                                    write_trade_to_bson(&kraken_trade);
                                    // printf("[TRADE] %s | %s | Price: %s | Size: %s\n", kraken_trade.exchange, kraken_trade.currency, kraken_trade.price, kraken_trade.size);
                                }
                            }
                        }
                        json_decref(root);
                        return 0;
                    }
                }
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
                   json_t *root, *obj, *b, *a, *c, *v, *p, *l, *h, *o;
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
                            // t = json_object_get(obj, "t");
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
                            if  (json_is_string(json_object_get(o, "o")))
                                strncpy(kraken_ticker.open_today, json_string_value(json_object_get(o, "o")), sizeof(kraken_ticker.open_today) - 1);       
        
                        }
                    }
                    if (extract_order_data((char *)in, "\"c\":[\"", kraken_ticker.price, sizeof(kraken_ticker.price)) &&
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
            else if (strncmp(protocol, "huobi-websocket", 15) == 0) {
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
                    else if (strstr(decompressed, "\"ch\":\"market.") && strstr(decompressed, ".trade.detail\"")) {
                        TradeData huobi_trade = {0};
                        strncpy(huobi_trade.exchange, "Huobi", sizeof(huobi_trade.exchange) - 1);

                        // Extract symbol from channel string
                        extract_huobi_currency(decompressed, huobi_trade.currency, sizeof(huobi_trade.currency));

                        // Extract trade details
                        extract_numeric(decompressed, "\"price\":", huobi_trade.price, sizeof(huobi_trade.price));
                        extract_numeric(decompressed, "\"amount\":", huobi_trade.size, sizeof(huobi_trade.size));
                        extract_numeric(decompressed, "\"ts\":", huobi_trade.timestamp, sizeof(huobi_trade.timestamp));
                        extract_numeric(decompressed, "\"id\":", huobi_trade.trade_id, sizeof(huobi_trade.trade_id));

                        char iso_ts[64] = {0};
                        convert_binance_timestamp(iso_ts, sizeof(iso_ts), huobi_trade.timestamp);
                        strncpy(huobi_trade.timestamp, iso_ts, sizeof(huobi_trade.timestamp) - 1);

                        log_trade_price(huobi_trade.timestamp, huobi_trade.exchange, huobi_trade.currency,
                                        huobi_trade.price, huobi_trade.size, huobi_trade.trade_id, huobi_trade.market_maker);

                        write_trade_to_bson(&huobi_trade);
                        // printf("[TRADE] %s | %s | Price: %s | Size: %s | ID: %s\n", huobi_trade.exchange, huobi_trade.currency, huobi_trade.price, huobi_trade.size, huobi_trade.trade_id);
                    }
                }
            }
            else if (strncmp(protocol, "okx-websocket", 13) == 0) {            
                // printf("[TICKER][OKX] %.*s\n", (int)len, (char *)in);

                TickerData okx_ticker = {0};
                strncpy(okx_ticker.exchange, "OKX", MAX_EXCHANGE_NAME_LENGTH - 1);
                okx_ticker.exchange[MAX_EXCHANGE_NAME_LENGTH - 1] = '\0'; 
                
                if (extract_order_data((char *)in, "\"last\":\"", okx_ticker.price, sizeof(okx_ticker.price)) &&
                    extract_order_data((char *)in, "\"instId\":\"", okx_ticker.currency, sizeof(okx_ticker.currency))) {

                    extract_order_data((char *)in, "\"bidPx\":\"", okx_ticker.bid, sizeof(okx_ticker.bid));
                    extract_order_data((char *)in, "\"bidSz\":\"", okx_ticker.bid_qty, sizeof(okx_ticker.bid_qty));
                    extract_order_data((char *)in, "\"askPx\":\"", okx_ticker.ask, sizeof(okx_ticker.ask));
                    extract_order_data((char *)in, "\"askSz\":\"", okx_ticker.ask_qty, sizeof(okx_ticker.ask_qty));

                    extract_order_data((char *)in, "\"open24h\":\"", okx_ticker.open_price, sizeof(okx_ticker.open_price));
                    extract_order_data((char *)in, "\"high24h\":\"", okx_ticker.high_price, sizeof(okx_ticker.high_price));
                    extract_order_data((char *)in, "\"low24h\":\"", okx_ticker.low_price, sizeof(okx_ticker.low_price));
                    extract_order_data((char *)in, "\"vol24h\":\"", okx_ticker.volume_24h, sizeof(okx_ticker.volume_24h));

                    if (!extract_order_data((char *)in, "\"ts\":\"", okx_ticker.timestamp, sizeof(okx_ticker.timestamp)))
                        get_timestamp(okx_ticker.timestamp, sizeof(okx_ticker.timestamp));
                    
                    log_ticker_price(&okx_ticker);
                    write_ticker_to_bson(&okx_ticker);
                } else if (strstr((char *)in, "\"arg\":{\"channel\":\"trades\"")) {
                    TradeData okx_trade = {0};
                    strncpy(okx_trade.exchange, "OKX", sizeof(okx_trade.exchange) - 1);

                    if (extract_order_data((char *)in, "\"px\":\"", okx_trade.price, sizeof(okx_trade.price)) &&
                        extract_order_data((char *)in, "\"instId\":\"", okx_trade.currency, sizeof(okx_trade.currency))) {

                        if (!extract_order_data((char *)in, "\"ts\":\"", okx_trade.timestamp, sizeof(okx_trade.timestamp))) {
                            get_timestamp(okx_trade.timestamp, sizeof(okx_trade.timestamp));
                        }

                        log_trade_price(okx_trade.timestamp, okx_trade.exchange, okx_trade.currency,
                                        okx_trade.price, okx_trade.size, okx_trade.trade_id, okx_trade.market_maker);
                        write_trade_to_bson(&okx_trade);
                        // printf("[TRADE] %s | %s | Price: %s | Time: %s\n", okx_trade.exchange, okx_trade.currency, okx_trade.price, okx_trade.timestamp);
                    }
                }
            }
            break;
        }
        case LWS_CALLBACK_CLIENT_CLOSED: {
            printf("[WARNING] %s WebSocket Connection Closed. Attempting Reconnect...\n", protocol);
            schedule_reconnect(protocol);
            break;
        }
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR: {
            printf("[ERROR] %s WebSocket Connection Error! Attempting Reconnect...\n", protocol);
            schedule_reconnect(protocol);
            break;
        }
        default: {
            break;
        }
    }
    
    return 0;
}



/* Write TickerData to a BSON file */
void write_ticker_to_bson(const TickerData *ticker) {
    time_t now = time(NULL);
    struct tm *tm = gmtime(&now);
    char filename[128];
    snprintf(filename, sizeof(filename), "bson_output/%s_ticker_%04d%02d%02d.bson",
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
    // } else {
        // printf("[INFO] Wrote TickerData to %s\n", filename);
    }

    bson_destroy(&doc);
    fclose(fp);
}

/* Write TradeData to a BSON file */
void write_trade_to_bson(const TradeData *trade) {
    time_t now = time(NULL);
    struct tm *tm = gmtime(&now);
    char filename[128];
    snprintf(filename, sizeof(filename), "bson_output/%s_trade_%04d%02d%02d.bson",
             trade->exchange, tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);

    FILE *fp = fopen(filename, "ab");
    if (!fp) {
        printf("[ERROR] Failed to open BSON file %s: %s\n", filename, strerror(errno));
        return;
    }

    bson_t doc;
    bson_init(&doc);

    BSON_APPEND_UTF8(&doc, "exchange", trade->exchange);
    BSON_APPEND_UTF8(&doc, "price", trade->price);
    BSON_APPEND_UTF8(&doc, "size", trade->size);
    BSON_APPEND_UTF8(&doc, "currency", trade->currency);
    BSON_APPEND_UTF8(&doc, "timestamp", trade->timestamp);
    BSON_APPEND_UTF8(&doc, "trade_id", trade->trade_id);
    BSON_APPEND_UTF8(&doc, "market_maker", trade->market_maker);

    const uint8_t *data = bson_get_data(&doc);
    if (fwrite(data, 1, doc.len, fp) != doc.len) {
        printf("[ERROR] Failed to write to BSON file %s\n", filename);
    }

    bson_destroy(&doc);
    fclose(fp);
}

/* Define the protocols array for use in the context. */
struct lws_protocols protocols[] = {
    { "binance-websocket-0", callback_combined, 0, 4096, 0, 0, 0 },
    { "binance-websocket-1", callback_combined, 0, 4096, 0, 0, 0 },
    { "binance-websocket-2", callback_combined, 0, 4096, 0, 0, 0 },
    { "binance-websocket-3", callback_combined, 0, 4096, 0, 0, 0 },
    { "binance-websocket-4", callback_combined, 0, 4096, 0, 0, 0 },
    { "binance-websocket-5", callback_combined, 0, 4096, 0, 0, 0 },
    { "coinbase-websocket", callback_combined, 0, 4096, 0, 0, 0 },
    { "kraken-websocket", callback_combined, 0, 4096, 0, 0, 0 },
    { "bitfinex-websocket", callback_combined, 0, 4096, 0, 0, 0 },
    { "huobi-websocket-0", callback_combined, 0, 4096, 0, 0, 0 },
    { "huobi-websocket-1", callback_combined, 0, 4096, 0, 0, 0 },
    { "huobi-websocket-2", callback_combined, 0, 4096, 0, 0, 0 },
    { "huobi-websocket-3", callback_combined, 0, 4096, 0, 0, 0 },
    { "huobi-websocket-4", callback_combined, 0, 4096, 0, 0, 0 },
    { "huobi-websocket-5", callback_combined, 0, 4096, 0, 0, 0 },
    { "huobi-websocket-6", callback_combined, 0, 4096, 0, 0, 0 },
    { "huobi-websocket-7", callback_combined, 0, 4096, 0, 0, 0 },
    { "huobi-websocket-8", callback_combined, 0, 4096, 0, 0, 0 },
    { "huobi-websocket-9", callback_combined, 0, 4096, 0, 0, 0 },
    { "huobi-websocket-10", callback_combined, 0, 4096, 0, 0, 0 },
    { "huobi-websocket-11", callback_combined, 0, 4096, 0, 0, 0 },
    { "huobi-websocket-12", callback_combined, 0, 4096, 0, 0, 0 },
    { "huobi-websocket-13", callback_combined, 0, 4096, 0, 0, 0 },
    { "huobi-websocket-14", callback_combined, 0, 4096, 0, 0, 0 },
    { "huobi-websocket-15", callback_combined, 0, 4096, 0, 0, 0 },
    { "huobi-websocket-16", callback_combined, 0, 4096, 0, 0, 0 },
    { "huobi-websocket-17", callback_combined, 0, 4096, 0, 0, 0 },
    { "huobi-websocket-18", callback_combined, 0, 4096, 0, 0, 0 },
    { "huobi-websocket-19", callback_combined, 0, 4096, 0, 0, 0 },
    { "okx-websocket-0", callback_combined, 0, 4096, 0, 0, 0 },
    { "okx-websocket-1", callback_combined, 0, 4096, 0, 0, 0 },
    { "okx-websocket-2", callback_combined, 0, 4096, 0, 0, 0 },
    { "okx-websocket-3", callback_combined, 0, 4096, 0, 0, 0 },
    { "okx-websocket-4", callback_combined, 0, 4096, 0, 0, 0 },
    { "okx-websocket-5", callback_combined, 0, 4096, 0, 0, 0 },
    { "okx-websocket-6", callback_combined, 0, 4096, 0, 0, 0 },
    { "okx-websocket-7", callback_combined, 0, 4096, 0, 0, 0 },
    { NULL, NULL, 0, 0, 0, 0, 0 }
};
