/*
 * Utility Functions
 * 
 * This module provides various utility functions for handling timestamps, logging 
 * market data.
 * 
 * Features:
 *  - Converts millisecond timestamps to ISO 8601 format.
 *  - Retrieves the current timestamp in ISO 8601 format with milliseconds.
 *  - Logs market price data in JSON format using Jansson.
 *  - Implements product mappings to standardize product symbols across exchanges.
 *  - Supports basic price comparison to resolve unknown product names.
 * 
 * Dependencies:
 *  - Jansson: Handles JSON serialization.
 *  - Standard C libraries (stdio.h, stdlib.h, string.h, time.h, math.h).
 * 
 * Usage:
 *  - Called by `exchange_websocket.c` for logging market data.
 * 
 * Created: 3/7/2025
 * Updated: 3/12/2025
 */

#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <jansson.h>
#include <sys/time.h>
#include <math.h>
#include <errno.h>
#include <zlib.h>

/* Global file pointer for log file */
FILE *ticker_data_file = NULL;
FILE *trades_data_file = NULL;

/* Mapping for product replacements */
static ProductMapping product_mappings_arr[] = {
    {"tBTCUSD", "BTC-USD"},
    {"BTCUSDT", "BTC-USD"},
    {"market.btcusdt", "BTC-USD"},
    {"BTC-USDT", "BTC-USD"},
    {"BTC/USD", "BTC-USD"},

    {"ADAUSDT", "ADA-USD"},
    {"ADA/USD", "ADA-USD"},

    {"ETHUSDT", "ETH-USD"},
    {"ETH/USD", "ETH-USD"},

    {"XBT/USD", "XBT-USD"},

    {NULL, NULL}
};

/* Convert any millisecond timestamp to ISO 8601 format */
void convert_binance_timestamp(char *timestamp_buffer, size_t buf_size, const char *ms_timestamp) {
    long long milliseconds = atoll(ms_timestamp);
    time_t seconds = milliseconds / 1000;
    struct tm t;
    gmtime_r(&seconds, &t);
    snprintf(timestamp_buffer, buf_size, "%04d-%02d-%02dT%02d:%02d:%02d.%03lldZ",
             t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
             t.tm_hour, t.tm_min, t.tm_sec, milliseconds % 1000);
}

/* Get the current timestamp in ISO 8601 format with milliseconds */
void get_timestamp(char *buffer, size_t buf_size) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm t;
    gmtime_r(&tv.tv_sec, &t);
    snprintf(buffer, buf_size, "%04d-%02d-%02dT%02d:%02d:%02d.%03ldZ",
             t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
             t.tm_hour, t.tm_min, t.tm_sec, tv.tv_usec / 1000);
}

/* Normalize and format any timestamp into "YYYY-MM-DD HH:MM:SS.ssssss UTC" */
int normalize_timestamp(const char *input, char *output, size_t output_size) {
    if (!input || !output) return 0;

    int is_digit = 1;
    for (const char *p = input; *p; p++) {
        if (!isdigit(*p)) {
            is_digit = 0;
            break;
        }
    }

    if (is_digit) {
        long long millis = atoll(input);
        time_t seconds = millis / 1000;
        int micros = (millis % 1000) * 1000;
        struct tm t;
        gmtime_r(&seconds, &t);
        snprintf(output, output_size,
                 "%04d-%02d-%02d %02d:%02d:%02d.%06d UTC",
                 t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                 t.tm_hour, t.tm_min, t.tm_sec, micros);
        return 1;
    }

    char date[11], time[16];
    if (sscanf(input, "%10[^T]T%15[^Z]", date, time) == 2) {
        snprintf(output, output_size, "%s %s UTC", date, time);
        return 1;
    }

    return 0;
}


/* Log price with provided timestamp, exchange, and currency in JSON format */
void log_ticker_price(const char *timestamp, const char *exchange, const char *currency, const char *price) {
    if (!ticker_data_file)
        return;
    // printf("[DEBUG] log_ticker_price() called for %s - %s | %s | %s\n", exchange, currency, price, timestamp);
    char mapped_currency[32];
    strncpy(mapped_currency, currency, sizeof(mapped_currency) - 1);
    mapped_currency[sizeof(mapped_currency) - 1] = '\0';

    for (ProductMapping *m = product_mappings_arr; m->key != NULL; m++) {
        if (strcmp(currency, m->key) == 0) {
            strncpy(mapped_currency, m->value, sizeof(mapped_currency) - 1);
            mapped_currency[sizeof(mapped_currency) - 1] = '\0';
            break;
        }
    }

    json_t *root = json_object();
    
    char formatted_timestamp[64];
    if (!normalize_timestamp(timestamp, formatted_timestamp, sizeof(formatted_timestamp))) {
        strncpy(formatted_timestamp, timestamp, sizeof(formatted_timestamp));
        formatted_timestamp[sizeof(formatted_timestamp) - 1] = '\0';
    }

    json_object_set_new(root, "timestamp", json_string(formatted_timestamp));    
    json_object_set_new(root, "exchange", json_string(exchange));
    json_object_set_new(root, "currency", json_string(mapped_currency));
    json_object_set_new(root, "price", json_string(price));

    char *json_str = json_dumps(root, 0);

    fprintf(ticker_data_file, "%s\n", json_str);
    
    free(json_str);
    json_decref(root);
    fflush(ticker_data_file);
}

/* Log trade price data with provided timestamp, exchange, currency, price, and size in JSON format */
void log_trade_price(const char *timestamp, const char *exchange, const char *currency, const char *price, const char *size) {
    if (!trades_data_file)
        return;
    // printf("[DEBUG] log_trade_price() called for %s - %s | %s | %s | %s\n", exchange, currency, price, size, timestamp);
    char mapped_currency[32];
    strncpy(mapped_currency, currency, sizeof(mapped_currency) - 1);
    mapped_currency[sizeof(mapped_currency) - 1] = '\0';

    for (ProductMapping *m = product_mappings_arr; m->key != NULL; m++) {
        if (strcmp(currency, m->key) == 0) {
            strncpy(mapped_currency, m->value, sizeof(mapped_currency) - 1);
            mapped_currency[sizeof(mapped_currency) - 1] = '\0';
            break;
        }
    }

    json_t *root = json_object();
    char formatted_timestamp[64];
    if (!normalize_timestamp(timestamp, formatted_timestamp, sizeof(formatted_timestamp))) {
        strncpy(formatted_timestamp, timestamp, sizeof(formatted_timestamp));
        formatted_timestamp[sizeof(formatted_timestamp) - 1] = '\0';
    }

    json_object_set_new(root, "timestamp", json_string(formatted_timestamp));
    json_object_set_new(root, "exchange", json_string(exchange));
    json_object_set_new(root, "currency", json_string(mapped_currency));
    json_object_set_new(root, "price", json_string(price));
    json_object_set_new(root, "size", json_string(size));

    char *json_str = json_dumps(root, 0);
    fprintf(trades_data_file, "%s\n", json_str);

    free(json_str);
    json_decref(root);
    fflush(trades_data_file);
}

/* Decompresses a Gzip-compressed input buffer into an output buffer using zlib. */
int decompress_gzip(const char *input, size_t input_len, char *output, size_t output_size) {
    z_stream strm = {0};
    strm.next_in = (Bytef *)input;
    strm.avail_in = input_len;
    strm.next_out = (Bytef *)output;
    strm.avail_out = output_size;

    if (inflateInit2(&strm, 16 + MAX_WBITS) != Z_OK) {
        return -1;
    }

    int result = inflate(&strm, Z_FINISH);
    inflateEnd(&strm);

    return (result == Z_STREAM_END) ? (int)strm.total_out : -1;
}