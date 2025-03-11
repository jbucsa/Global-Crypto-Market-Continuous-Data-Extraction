/*
 * Utility Functions for WebSocket Market Data Processing
 * 
 * This module provides various utility functions for handling timestamps, logging 
 * market data, and processing CSV files.
 * 
 * Features:
 *  - Converts millisecond timestamps to ISO 8601 format.
 *  - Retrieves the current timestamp in ISO 8601 format with milliseconds.
 *  - Logs market price data in JSON format using Jansson.
 *  - Parses and processes market data stored in CSV format.
 *  - Implements product mappings to standardize product symbols across exchanges.
 *  - Supports basic price comparison to resolve unknown product names.
 * 
 * Dependencies:
 *  - Jansson: Handles JSON serialization.
 *  - Standard C libraries (stdio.h, stdlib.h, string.h, time.h, math.h).
 * 
 * Usage:
 *  - Called by `exchange_websocket.c` for logging market data.
 *  - Used in `main.c` for processing CSV market data.
 * 
 * Created: 3/7/2025
 * Updated: 3/11/2025
 */

#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <jansson.h>
#include <sys/time.h>
#include <math.h>

/* Global file pointer for log file */
FILE *ticker_data_file = NULL;

/* Mapping for product replacements */
static ProductMapping product_mappings_arr[] = {
    {"tBTCUSD", "BTC-USD"},
    {"BTCUSDT", "BTC-USD"},
    {"ADAUSDT", "ADA-USD"},
    {"ETHUSDT", "ETH-USD"},
    {NULL, NULL}
};

/* Price counters for tracking recent prices */
static PriceCounter price_counters_arr[] = {
    {"ADA-USD", 0.0, 0},
    {"BTC-USD", 0.0, 0},
    {"ETH-USD", 0.0, 0}
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

/* Utility function to get the current timestamp in ISO 8601 format with milliseconds */
void get_timestamp(char *buffer, size_t buf_size) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm t;
    gmtime_r(&tv.tv_sec, &t);
    snprintf(buffer, buf_size, "%04d-%02d-%02dT%02d:%02d:%02d.%03ldZ",
             t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
             t.tm_hour, t.tm_min, t.tm_sec, tv.tv_usec / 1000);
}

/* Log price with provided timestamp, exchange, and currency in JSON format */
void log_ticker_price(const char *timestamp, const char *exchange, const char *currency, const char *price) {
    if (!ticker_data_file)
        return;

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
    json_object_set_new(root, "timestamp", json_string(timestamp));
    json_object_set_new(root, "exchange", json_string(exchange));
    json_object_set_new(root, "currency", json_string(mapped_currency));
    json_object_set_new(root, "price", json_string(price));

    char *json_str = json_dumps(root, JSON_INDENT(4));

    fprintf(ticker_data_file, "%s,\n", json_str);
    
    free(json_str);
    json_decref(root);
    fflush(ticker_data_file);
}

/* CSV Processing Functions */
int compare_entries_csv(const void *a, const void *b) {
    return strcmp(((EntryCSV *)a)->timestamp, ((EntryCSV *)b)->timestamp);
}

void free_entry_csv(EntryCSV *entry) {
    free(entry->timestamp);
    free(entry->exchange);
    free(entry->product);
}

EntryCSV *parse_line_csv(char *line) {
    char *tokens[4] = {NULL};
    char *ptr = strtok(line, "[]");
    for (int i = 0; i < 4 && ptr; i++, ptr = strtok(NULL, "[]")) {
        tokens[i] = ptr;
    }
    if (!tokens[3]) return NULL;

    EntryCSV *entry = malloc(sizeof(EntryCSV));
    entry->timestamp = strdup(tokens[0]);
    entry->exchange = strdup(tokens[1]);
    entry->product = strdup(tokens[2]);
    entry->price = strtod(tokens[3] + 7, NULL);
    
    return entry;
}

/* Processes market data, applies mappings, and writes to a CSV file. */
void process_data_and_write_csv(EntryCSV *entries, size_t num_entries, const char *output_file) {
    qsort(entries, num_entries, sizeof(EntryCSV), compare_entries_csv);

    for (size_t i = 0; i < num_entries; i++) {
        EntryCSV *entry = &entries[i];

        for (ProductMapping *m = product_mappings_arr; m->key; m++) {
            if (strcmp(entry->product, m->key) == 0) {
                free(entry->product);
                entry->product = strdup(m->value);
                break;
            }
        }

        for (int j = 0; j < 3; j++) {
            if (strcmp(entry->product, price_counters_arr[j].product) == 0) {
                price_counters_arr[j].value = entry->price;
                price_counters_arr[j].initialized = 1;
                break;
            }
        }

        if (strcmp(entry->product, "unknown") == 0) {
            double min_diff = INFINITY;
            int closest = -1;
            for (int j = 0; j < 3; j++) {
                if (price_counters_arr[j].initialized) {
                    double diff = fabs(entry->price - price_counters_arr[j].value);
                    if (diff < min_diff) {
                        min_diff = diff;
                        closest = j;
                    }
                }
            }
            if (closest != -1) {
                free(entry->product);
                entry->product = strdup(price_counters_arr[closest].product);
            }
        }
    }

    FILE *output = fopen(output_file, "w");
    if (!output) {
        perror("Error opening output file");
        return;
    }
    fprintf(output, "index,time,exchange,product,price\n");
    for (size_t i = 0; i < num_entries; i++) {
        fprintf(output, "%zu,%s,%s,%s,%.8f\n", i + 1, entries[i].timestamp, 
                entries[i].exchange, entries[i].product, entries[i].price);
    }
    fclose(output);
}

/* Reads and processes CSV market data, then writes the results to an output file. */
void run_csv_processing_mode(const char *input_file, const char *output_file) {
    FILE *input = fopen(input_file, "r");
    if (!input) {
        perror("Error opening input file");
        exit(1);
    }
    EntryCSV *entries = NULL;
    size_t num_entries = 0, capacity = 16;
    entries = malloc(capacity * sizeof(EntryCSV));
    if (!entries) {
        perror("Memory allocation failed");
        exit(1);
    }

    char *line = NULL;
    size_t len = 0;
    while (getline(&line, &len, input) != -1) {
        char *trimmed_line = line;
        while (*trimmed_line == ' ' || *trimmed_line == '\t') trimmed_line++;
        size_t linelen = strlen(trimmed_line);
        while (linelen > 0 && (trimmed_line[linelen - 1] == '\n' || trimmed_line[linelen - 1] == '\r'))
            trimmed_line[--linelen] = '\0';
        if (linelen == 0) continue;

        EntryCSV *entry = parse_line_csv(trimmed_line);
        if (!entry) {
            fprintf(stderr, "Skipping invalid line: %s\n", trimmed_line);
            continue;
        }
        if (num_entries >= capacity) {
            capacity *= 2;
            entries = realloc(entries, capacity * sizeof(EntryCSV));
            if (!entries) {
                perror("Memory allocation failed");
                exit(1);
            }
        }
        entries[num_entries++] = *entry;
        free(entry);
    }
    free(line);
    fclose(input);
    
    process_data_and_write_csv(entries, num_entries, output_file);
    
    for (size_t i = 0; i < num_entries; i++) {
        free_entry_csv(&entries[i]);
    }
    free(entries);
}
