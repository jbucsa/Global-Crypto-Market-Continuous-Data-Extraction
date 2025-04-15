/*
 * Utility Functions Header
 * 
 * This header file declares utility functions and data structures for managing
 * timestamps, handling compressed data, and logging cryptocurrency market data 
 * received via exchange WebSocket connections.
 * 
 * Functionality:
 *  - convert_binance_timestamp(): Converts Binance millisecond timestamps to ISO 8601 format.
 *  - get_timestamp(): Returns the current timestamp in ISO 8601 format.
 *  - log_ticker_price(): Logs ticker data in JSON format.
 *  - log_trade_price(): Logs trade data in JSON format.
 *  - decompress_gzip(): Decompresses Gzip data buffers.
 *  - flush_buffer_to_file(): Flushes buffered JSON to file.
 *  - init_json_buffers(): Initializes JSON buffers for ticker and trade data.
 * 
 * Structures:
 *  - ProductMapping: Maps exchange-specific product symbols to standardized names.
 *  - PriceCounter: Tracks price changes for unknown product detection.
 * 
 * Dependencies:
 *  - Standard C libraries (stddef.h, stdio.h).
 *  - External: jansson (for JSON manipulation).
 * 
 * Usage:
 *  - Implemented in `utils.c`.
 *  - Called from `exchange_websocket.c`, `main.c`, and related modules.
 * 
 * Created: 03/07/2025
 * Updated: 04/14/2025
 */

 #ifndef UTILS_H
 #define UTILS_H
 
 #include <stddef.h>
 #include <stdio.h>
 #include <jansson.h>
 
 /* ---------------------------- File Logging ---------------------------- */
 
 /* Global file pointers used for writing ticker and trade data */
 extern FILE *ticker_data_file;
 extern FILE *trades_data_file;
 
 /* -------------------------- Timestamp Utilities ----------------------- */
 
 /* Converts a millisecond-based timestamp string into ISO 8601 format and stores it in the given buffer. */
 void convert_binance_timestamp(char *timestamp_buffer, size_t buf_size, const char *ms_timestamp);
 
 /* Populates a buffer with the current timestamp in ISO 8601 format. */
 void get_timestamp(char *buffer, size_t buf_size);
 
 /* ---------------------------- Logging Helpers ------------------------- */
 
 /* Logs ticker data in JSON format using timestamp, exchange, currency, and price. */
 void log_ticker_price(const char *timestamp, const char *exchange, const char *currency, const char *price);
 
 /* Logs trade data in JSON format using timestamp, exchange, currency, price, and size. */
 void log_trade_price(const char *timestamp, const char *exchange, const char *currency, const char *price, const char *size);
 
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
 