/*
 * Utility Functions Header
 * 
 * Declares utility functions and structures for timestamp handling, JSON logging,
 * buffer management, Gzip decompression, and product symbol normalization.
 * 
 * Features:
 *  - convert_binance_timestamp(): Converts millisecond timestamps to ISO 8601.
 *  - get_timestamp(): Returns the current UTC timestamp with milliseconds.
 *  - log_ticker_price(): Logs ticker-level JSON entries.
 *  - log_trade_price(): Logs trade-level JSON entries.
 *  - decompress_gzip(): Inflates compressed WebSocket payloads.
 *  - flush_buffer_to_file(): Writes buffered JSON to disk.
 *  - init_json_buffers(): Loads recent entries from previous session.
 * 
 * Structures:
 *  - ProductMapping: Symbol translation map for exchange data.
 *  - PriceCounter: Tracks unknown price patterns (future use).
 * 
 * Dependencies:
 *  - stdio.h, stddef.h, time.h: Standard C headers.
 *  - jansson: For JSON manipulation.
 * 
 * Usage:
 *  - Used by exchange_websocket.c, main.c, and reconnect logic.
 * 
 * Created: 3/7/2025
 * Updated: 5/4/2025
 */

 #ifndef UTILS_H
 #define UTILS_H
 
 #include <stddef.h>
 #include <stdio.h>
 #include <jansson.h>

 #include "exchange_websocket.h"
 
 /* ---------------------------- File Logging ---------------------------- */
 
 /* Global file pointers used for writing ticker and trade data */
 extern FILE *ticker_data_file;
 extern FILE *trades_data_file;

 int count_symbols_in_file(const char *filename);
 
 /* -------------------------- Timestamp Utilities ----------------------- */
 
 /* Converts a millisecond-based timestamp string into ISO 8601 format and stores it in the given buffer. */
 void convert_binance_timestamp(char *timestamp_buffer, size_t buf_size, const char *ms_timestamp);
 
 /* Populates a buffer with the current timestamp in ISO 8601 format. */
 void get_timestamp(char *buffer, size_t buf_size);
 
 /* ---------------------------- Logging Helpers ------------------------- */
 
 /* Logs ticker data in JSON format using timestamp, exchange, currency, and price. */
 void log_ticker_price(TickerData *ticker_data);
 
 /* Logs trade data in JSON format using timestamp, exchange, currency, price, and size. */
 void log_trade_price(const char *timestamp, const char *exchange, const char *currency, const char *price, const char *size, const char *trade_id, const char *market_maker);
 
 /* Flushes a JSON buffer to a file, used for efficient batch writing. */
 void flush_buffer_to_file(const char *filename, json_t *buffer);
 
 /* Initializes global JSON buffers used for ticker and trade data. */
 void init_json_buffers();
 
 /* ---------------------------- JSON Buffers ---------------------------- */
 
 /* Global JSON buffers used for batch log flushing */
 extern json_t *ticker_buffer;
 extern json_t *trades_buffer;
 
 /* ------------------------- Data Structures ---------------------------- */
 
 /* Represents a mapping between an exchange-specific key and its standardized equivalent. */
 typedef struct {
     char *key;
     char *value;
 } ProductMapping;
 
 /* Tracks price state for a specific product, used in identifying unknown products or filtering noise. */
 typedef struct {
     char *product;
     double value;
     int initialized;
 } PriceCounter;
 
 /* ------------------------- Compression Utils -------------------------- */
 
 /* Decompresses a Gzip-compressed buffer into a readable string using zlib. */
 int decompress_gzip(const char *input, size_t input_len, char *output, size_t output_size);
 
 #endif // UTILS_H
 