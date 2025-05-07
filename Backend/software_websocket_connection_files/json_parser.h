/*
 * JSON Parser Header
 * 
 * This header file declares functions used for extracting data from JSON-formatted
 * WebSocket messages received from cryptocurrency exchanges.
 * 
 * Functionality:
 *  - `extract_order_data()`: Extracts a quoted string value (e.g., price) from JSON.
 *  - `extract_numeric()`: Extracts a numeric (unquoted) value from JSON.
 *  - `extract_bitfinex_price()`: Extracts ticker price from a Bitfinex array message.
 *  - `extract_huobi_currency()`: Extracts currency identifiers from Huobi's channel string.
 * 
 * Dependencies:
 *  - Standard C library (stddef.h) for size definitions.
 * 
 * Usage:
 *  - Implemented in `json_parser.c`.
 *  - Used in `exchange_websocket.c` for parsing WebSocket market data.
 * 
 * Created: 3/7/2025
 * Updated: 3/11/2025
 */

#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include <stddef.h>

/* Extract a quoted value from JSON using the specified key */
int extract_order_data(const char *json, const char *key, char *dest, size_t dest_size);

/* Extract a numeric (unquoted) value from JSON using the specified key */
int extract_numeric(const char *json, const char *key, char *dest, size_t dest_size);

/* Extract Bitfinex ticker price from an array message */
int extract_bitfinex_price(const char *json, char *dest, size_t dest_size);

/* Extract currency from Huobi channel string */
int extract_huobi_currency(const char *json, char *dest, size_t dest_size);

#endif // JSON_PARSER_H
