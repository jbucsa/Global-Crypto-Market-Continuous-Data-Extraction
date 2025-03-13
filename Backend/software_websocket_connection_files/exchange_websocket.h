/*
 * Exchange WebSocket Header
 * 
 * This header file declares functions and global variables used for managing 
 * WebSocket connections to multiple cryptocurrency exchanges.
 * 
 * Functionality:
 *  - Declares `callback_combined()`, a unified WebSocket callback function 
 *    that processes events such as connection, message reception, and disconnection.
 *  - Declares the global `protocols[]` array, which defines WebSocket protocols 
 *    for supported exchanges.
 * 
 * Dependencies:
 *  - libwebsockets: Provides WebSocket functionality.
 * 
 * Usage:
 *  - Included in `exchange_websocket.c` for implementation.
 *  - Included in `main.c` for initializing WebSocket connections.
 * 
 * Created: 3/7/2025
 * Updated: 3/11/2025
 */

#ifndef EXCHANGE_WEBSOCKET_H
#define EXCHANGE_WEBSOCKET_H

#include <libwebsockets.h>

/* Callback function for handling WebSocket events. */
int callback_combined(struct lws *wsi, enum lws_callback_reasons reason,
                      void *user, void *in, size_t len);

/* Global protocols array (defined in exchange_websocket.c) */
extern struct lws_protocols protocols[];

/* Global file pointer for writing market data. */
extern FILE *ticker_data_file;
extern FILE *trades_data_file;

#endif // EXCHANGE_WEBSOCKET_H
