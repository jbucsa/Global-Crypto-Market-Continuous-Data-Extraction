/*
 * Exchange Connect Header
 * 
 * This header file declares functions for establishing WebSocket connections 
 * to multiple cryptocurrency exchanges.
 * 
 * Functionality:
 *  - Provides function prototypes for initiating WebSocket connections 
 *    for supported exchanges.
 * 
 * Dependencies:
 *  - libwebsockets: Handles WebSocket connections.
 * 
 * Usage:
 *  - Included in `exchange_connect.c` for implementation.
 *  - Included in `main.c` and `exchange_reconnect.c` for connection handling.
 * 
 * Created: 3/11/2025
 * Updated: 3/11/2025
 */

#ifndef EXCHANGE_CONNECT_H
#define EXCHANGE_CONNECT_H

#include <libwebsockets.h>

/* Function prototypes for establishing WebSocket connections */
void connect_to_binance();
void connect_to_coinbase();
void connect_to_kraken();
void connect_to_bitfinex();
void connect_to_huobi();
void connect_to_okx();
void start_exchange_connections();

#endif // EXCHANGE_CONNECT_H
