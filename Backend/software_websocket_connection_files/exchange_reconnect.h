/*
 * Exchange Reconnect Header
 * 
 * Declares structures and functions for managing reconnections to WebSocket 
 * endpoints for cryptocurrency exchanges after timeouts or failures.
 * 
 * Features:
 *  - Tracks retry attempts and message timestamps per exchange.
 *  - Supports scheduled reconnection with incremental backoff.
 *  - Launches a health monitor thread to check connection status.
 * 
 * Dependencies:
 *  - Standard C libraries (time.h).
 * 
 * Usage:
 *  - Included by `exchange_reconnect.c` and used in `exchange_websocket.c`.
 * 
 * Created: 3/11/2025
 * Updated: 5/4/2025
 */

#include <time.h>

#ifndef EXCHANGE_RECONNECT_H
#define EXCHANGE_RECONNECT_H

/* Maximum number of supported exchanges */
#define MAX_EXCHANGES 25

/* Structure to store retry count per exchange */
typedef struct {
    const char *exchange;
    int retry_count;
} ExchangeRetry;

/* Declare retry_counts as a global variable */
extern ExchangeRetry retry_counts[MAX_EXCHANGES];

/* Track last message time */
extern time_t last_message_time[MAX_EXCHANGES];

/* Function prototypes */
int get_exchange_index(const char *exchange);
void schedule_reconnect(const char *exchange);

/* Global WebSocket context */
void start_health_monitor();

#endif // EXCHANGE_RECONNECT_H
