/*
 * Crypto Exchange WebSocket
 * 
 * This program connects to multiple cryptocurrency exchanges via WebSocket APIs
 * to receive real-time market data, extract relevant information, and log the
 * prices along with timestamps to a .json file. It also includes a CSV processing 
 * mode that allows converting logged JSON data into a structured CSV format.
 * 
 * Supported Exchanges:
 *  - Binance (Full)
 *  - Coinbase (Full)
 *  - Kraken
 *  - Bitfinex
 *  - Huobi
 *  - OKX
 * 
 * The program uses the libwebsockets library to establish WebSocket connections.
 * It implements callbacks for handling WebSocket events such as connection
 * establishment, message reception, and disconnection.
 * 
 * Features:
 *  - Converts Binance millisecond timestamps to ISO 8601 format.
 *  - Extracts and logs price data from JSON messages.
 *  - Supports multiple WebSocket connections concurrently.
 *  - Writes log data in the format: [timestamp][exchange][currency] Price: value
 *  - Provides an optional CSV processing mode for structured data export.
 * 
 * Dependencies:
 *  - libwebsockets : Links the libwebsockets library, required for establishing 
 *    and handling WebSocket connections with exchanges.
 *  - jansson : Links the Jansson library, which is used for handling JSON data.
 *  - lm : Links the math library (libm), needed for mathematical operations 
 *    (used for price comparisons).
 *  - Standard C libraries (stdio, stdlib, string, time, sys/time).
 * 
 * Usage (See makefile for compilation instructions):
 *    Run the program:
 *      "./crypto_ws"
 * 
 * Created: 3/7/2025
 * Updated: 3/11/2025
 */


 #include <stdio.h>
 #include <string.h>
 #include <stdlib.h>
 #include <libwebsockets.h>
 
/* Structure to hold WebSocket connection configuration */
struct connection_config {
    const char *name;
    const char *address;
    int port;
    const char *path;
    const char *host;
    const char *origin;
    int protocol_index;
};

/* Function to run the CSV processing mode */
void run_csv_processing_mode(const char *input_file, const char *output_file);

/* External declaration of WebSocket protocols */
extern const struct lws_protocols protocols[];

/* External declaration of the data file pointer */
extern FILE *ticker_data_file;

 /* main function handles both CSV processing and WebSocket */
 int main(int argc, char *argv[]) {
     if (argc == 3) {
         run_csv_processing_mode(argv[1], argv[2]);
         return 0;
     }
     struct lws_context *context;
     struct lws_context_creation_info context_info;
     memset(&context_info, 0, sizeof(context_info));
     context_info.port = CONTEXT_PORT_NO_LISTEN;
     context_info.protocols = protocols;
     context_info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
     context = lws_create_context(&context_info);
     if (!context) {
         printf("[ERROR] Failed to create WebSocket context\n");
         return -1;
     }
     ticker_data_file = fopen("ticker_output_data.json", "a");
     if (!ticker_data_file) {
         printf("[ERROR] Failed to open log file\n");
         lws_context_destroy(context);
         return -1;
     }
     struct connection_config configs[] = {
         {"Binance", "stream.binance.us", 9443, "/stream?streams=btcusdt@trade/btcusdt@ticker/ethusdt@trade/ethusdt@ticker/adausdt@trade/adausdt@ticker", "stream.binance.us", "stream.binance.us", 0},
         {"Coinbase", "ws-feed.exchange.coinbase.com", 443, "/", "ws-feed.exchange.coinbase.com", "ws-feed.exchange.coinbase.com", 1},
         {"Kraken", "ws.kraken.com", 443, "/", "ws.kraken.com", "ws.kraken.com", 2},
         {"Bitfinex", "api-pub.bitfinex.com", 443, "/ws/2", "api-pub.bitfinex.com", "api-pub.bitfinex.com", 3},
         {"Huobi", "api.huobi.pro", 443, "/ws", "api.huobi.pro", "api.huobi.pro", 4},
         {"OKX", "ws.okx.com", 8443, "/ws/v5/public", "ws.okx.com", "ws.okx.com", 5}
     };
     int i;
     for (i = 0; i < (int)(sizeof(configs) / sizeof(configs[0])); i++) {
         struct lws_client_connect_info conn_info;
         memset(&conn_info, 0, sizeof(conn_info));
         conn_info.context = context;
         conn_info.address = configs[i].address;
         conn_info.port = configs[i].port;
         conn_info.path = configs[i].path;
         conn_info.host = configs[i].host;
         conn_info.origin = configs[i].origin;
         conn_info.protocol = protocols[configs[i].protocol_index].name;
         conn_info.ssl_connection = LCCSCF_USE_SSL;
         if (!lws_client_connect_via_info(&conn_info))
             printf("[ERROR] Failed to connect to %s WebSocket server\n", configs[i].name);
     }
     while (lws_service(context, 1000) >= 0) {}
     printf("[INFO] Cleaning up WebSocket context...\n");
     fclose(ticker_data_file);
     lws_context_destroy(context);
     return 0;
 }
 