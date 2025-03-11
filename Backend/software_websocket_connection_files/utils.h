/*
 * Utility Functions Header
 * 
 * This header file declares functions and structures for handling timestamps, 
 * logging market data, and processing CSV files related to cryptocurrency 
 * exchange WebSocket data.
 * 
 * Functionality:
 *  - `convert_binance_timestamp()`: Converts a millisecond timestamp to ISO 8601 format.
 *  - `get_timestamp()`: Retrieves the current timestamp in ISO 8601 format.
 *  - `log_ticker_price()`: Logs exchange ticker price data in JSON format.
 *  - `compare_entries_csv()`: Compares CSV entries for sorting.
 *  - `parse_line_csv()`: Parses market data from a CSV log entry.
 *  - `process_data_and_write_csv()`: Processes and formats logged market data into CSV.
 *  - `run_csv_processing_mode()`: Reads input JSON and converts it to CSV format.
 * 
 * Structures:
 *  - `EntryCSV`: Represents a structured market data entry.
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
 * Updated: 3/11/2025
 */

#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdio.h>

/* Global file pointer for logging data */
extern FILE *ticker_data_file;

/* Timestamp conversion and logging utilities */
void convert_binance_timestamp(char *timestamp_buffer, size_t buf_size, const char *ms_timestamp);
void get_timestamp(char *buffer, size_t buf_size);
void log_ticker_price(const char *timestamp, const char *exchange, const char *currency, const char *price);

/* CSV processing related types and functions */
typedef struct {
    char *timestamp;
    char *exchange;
    char *product;
    double price;
} EntryCSV;

typedef struct {
    char *key;
    char *value;
} ProductMapping;

typedef struct {
    char *product;
    double value;
    int initialized;
} PriceCounter;

/* Compares two CSV entries by timestamp for sorting. */
int compare_entries_csv(const void *a, const void *b);

/* Frees memory allocated for a CSV entry. */
void free_entry_csv(EntryCSV *entry);

/* Parses a single line from a CSV file into an EntryCSV structure. */
EntryCSV *parse_line_csv(char *line);

/* Processes market data, applies mappings, and writes to a CSV file. */
void process_data_and_write_csv(EntryCSV *entries, size_t num_entries, const char *output_file);

/* Reads and processes CSV market data, then writes the results to an output file. */
void run_csv_processing_mode(const char *input_file, const char *output_file);

#endif // UTILS_H
