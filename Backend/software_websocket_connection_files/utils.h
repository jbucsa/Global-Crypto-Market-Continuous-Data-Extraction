/*
 * Utility Functions Header
 * 
 * This header file declares functions and structures for handling timestamps, 
 * and logging market data related to cryptocurrency 
 * exchange WebSocket data.
 * 
 * Functionality:
 *  - `convert_binance_timestamp()`: Converts a millisecond timestamp to ISO 8601 format.
 *  - `get_timestamp()`: Retrieves the current timestamp in ISO 8601 format.
 *  - `log_ticker_price()`: Logs exchange ticker price data in JSON format.
 *  - `decompress_gzip()`: Converts data from compressed to readable format.
 * 
 * Structures:
 *  - `ProductMapping`: Stores mappings between exchange-specific and standardized symbols.
 *  - `PriceCounter`: Tracks price data for identifying unknown product symbols.
 * 
 * Dependencies:
 *  - Standard C libraries (stddef.h, stdio.h).
 * 
 * Usage:
 *  - Implemented in `utils.c`.
 *  - Used by `exchange_websocket.c` and `main.c` for logging and data processing.
 * 
 * Created: 3/7/2025
 * Updated: 3/12/2025
 */

#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdio.h>

/* Global file pointer for logging data */
extern FILE *ticker_data_file;
extern FILE *trades_data_file;

/* Timestamp conversion and logging utilities */
void convert_binance_timestamp(char *timestamp_buffer, size_t buf_size, const char *ms_timestamp);
void get_timestamp(char *buffer, size_t buf_size);
void log_ticker_price(const char *timestamp, const char *exchange, const char *currency, const char *price);
void log_trade_price(const char *timestamp, const char *exchange, const char *currency, const char *price, const char *size);

typedef struct {
    char *key;
    char *value;
} ProductMapping;

typedef struct {
    char *product;
    double value;
    int initialized;
} PriceCounter;

/* Decompresses a Gzip-compressed input buffer into an output buffer using zlib. */
int decompress_gzip(const char *input, size_t input_len, char *output, size_t output_size);

#endif // UTILS_H
