/*
 * Exchange Reconnect Header
 * 
 * This header file defines functions and structures for handling WebSocket 
 * reconnections to cryptocurrency exchanges in case of connection failures.
 * 
 * Functionality:
 *  - Maintains per-exchange retry counters for reconnection attempts.
 *  - Provides functions to schedule reconnects with increasing delays.
 * 
 * Dependencies:
 *  - Standard C libraries (stdio, string, unistd).
 * 
 * Usage:
 *  - Included in `exchange_reconnect.c` for implementation.
 *  - Used in `exchange_websocket.c` to manage reconnection logic.
 * 
 * Created: 3/11/2025
 * Updated: 3/11/2025
 */

#ifndef EXCHANGE_RECONNECT_H
#define EXCHANGE_RECONNECT_H

/* Maximum number of supported exchanges */
#define MAX_EXCHANGES 6

/* Structure to store retry count per exchange */
typedef struct {
    const char *exchange;
    int retry_count;
} ExchangeRetry;

/* Declare retry_counts as a global variable */
extern ExchangeRetry retry_counts[MAX_EXCHANGES];

/* Function prototypes */
int get_exchange_index(const char *exchange);
void schedule_reconnect(const char *exchange);

#endif // EXCHANGE_RECONNECT_H
