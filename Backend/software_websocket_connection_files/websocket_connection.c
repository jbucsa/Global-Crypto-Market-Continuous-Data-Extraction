/*
 * Crypto Exchange WebSocket
 * 
 * This program connects to multiple cryptocurrency exchanges via WebSocket APIs
 * to receive real-time market data, extract relevant information, and log the
 * prices along with timestamps to a .json file.
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
 * 
 * Dependencies:
 *  - libwebsockets : Links the libwebsockets library, required for establishing and handling WebSocket connections with exchanges
 *  - ljansson : Links the Jansson library, which is used for handling JSON data
 *  - lm : Links the math library (libm), needed for mathematical operations (used for price comparisons)
 *  - Standard C libraries (stdio, stdlib, string, time, sys/time)
 * 
 * Usage:
 *  - Compile using a C compiler with the required dependencies: 
 *    "gcc -o websocket_connection websocket_connection.c -ljansson -lwebsockets -lm"
 *  - Run the executable to establish WebSocket connections and start logging: "./websocket_connection"
 * 
 * Created: 2/26/2025
 * Updated: 3/6/2025
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <time.h>
 #include <jansson.h>
 #include <sys/time.h>
 #include <libwebsockets.h>
 #include <math.h>
 

 FILE *data_file = NULL;

  /* Entry structure is reused for CSV processing */
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

/* Mapping for product replacements */
ProductMapping product_mappings_arr[] = {
    {"tBTCUSD", "BTC-USD"},
    {"BTCUSDT", "BTC-USD"},
    {"ADAUSDT", "ADA-USD"},
    {"ETHUSDT", "ETH-USD"},
    {NULL, NULL}
};

/* Price counters for tracking recent prices */
PriceCounter price_counters_arr[] = {
    {"ADA-USD", 0.0, 0},
    {"BTC-USD", 0.0, 0},
    {"ETH-USD", 0.0, 0}
};
 
 /* Convert any millisecond timestamp to ISO 8601 format */
 static void convert_binance_timestamp(char *timestamp_buffer, size_t buf_size, const char *ms_timestamp) {
     long long milliseconds = atoll(ms_timestamp);
     time_t seconds = milliseconds / 1000;
     long ms = milliseconds % 1000;
     struct tm t;
     gmtime_r(&seconds, &t);
     snprintf(timestamp_buffer, buf_size, "%04d-%02d-%02dT%02d:%02d:%02d.%03ldZ",
              t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
              t.tm_hour, t.tm_min, t.tm_sec, ms);
 }
 
 /* Utility function to get the current timestamp in ISO 8601 format with milliseconds. */
 static void get_timestamp(char *buffer, size_t buf_size) {
     struct timeval tv;
     gettimeofday(&tv, NULL); 
 
     struct tm t;
     gmtime_r(&tv.tv_sec, &t);
 
     snprintf(buffer, buf_size, "%04d-%02d-%02dT%02d:%02d:%02d.%03ldZ",
              t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
              t.tm_hour, t.tm_min, t.tm_sec, tv.tv_usec / 1000);
 }
 
 /* Extract a quoted value from JSON using key */
 static int extract_price(const char *json, const char *key, char *dest, size_t dest_size) {
     char *pos = strstr(json, key);
     if (!pos)
         return 0;
     pos += strlen(key);
     size_t i = 0;
     while (*pos && *pos != '"' && i < dest_size - 1) {
         dest[i++] = *pos++;
     }
     dest[i] = '\0';
     return 1;
 }
 
 /* Extract a numeric (unquoted) value from JSON using key */
 static int extract_numeric(const char *json, const char *key, char *dest, size_t dest_size) {
     char *pos = strstr(json, key);
     if (!pos)
         return 0;
     pos += strlen(key);
     while (*pos && (*pos == ' ' || *pos == ':' || *pos == '\"'))
         pos++;
     size_t i = 0;
     while (*pos && (((*pos >= '0' && *pos <= '9') || *pos == '.' || *pos == '-')) && i < dest_size - 1) {
         dest[i++] = *pos++;
     }
     dest[i] = '\0';
     return 1;
 }
 
 /* Extract Bitfinex ticker price from an array message. */
 static int extract_bitfinex_price(const char *json, char *dest, size_t dest_size) {
     const char *start = strchr(json, '[');
     if (!start)
         return 0;
     start++;
     int commaCount = 0;
     const char *p = start;
     const char *priceStart = NULL;
     while (*p) {
         if (*p == ',') {
             commaCount++;
             if (commaCount == 7) {
                 priceStart = p + 1;
                 break;
             }
         }
         p++;
     }
     if (!priceStart)
         return 0;
     size_t i = 0;
     while (*priceStart && *priceStart != ',' && *priceStart != ']' && i < dest_size - 1) {
         dest[i++] = *priceStart++;
     }
     dest[i] = '\0';
     return 1;
 }
 
 /* Extract currency from Huobi channel string. */
 static void extract_huobi_currency(const char *json, char *dest, size_t dest_size) {
     const char *ch_pos = strstr(json, "\"ch\":\"");
     if (!ch_pos) {
         strncpy(dest, "unknown", dest_size);
         return;
     }
     ch_pos += strlen("\"ch\":\"");
     const char *end = strstr(ch_pos, ".ticker");
     if (!end) {
         strncpy(dest, "unknown", dest_size);
         return;
     }
     size_t len = end - ch_pos;
     if (len >= dest_size)
         len = dest_size - 1;
     strncpy(dest, ch_pos, len);
     dest[len] = '\0';
 }
 
 /* Log price with provided timestamp, exchange, and currency in JSON format using Jansson */
 static void log_price(const char *timestamp, const char *exchange, const char *currency, const char *price) {
    if (!data_file)
        return;

    char mapped_currency[32];
    strncpy(mapped_currency, currency, sizeof(mapped_currency) - 1);
    mapped_currency[sizeof(mapped_currency) - 1] = '\0';

    for (ProductMapping *m = product_mappings_arr; m->key != NULL; m++) {
        if (strcmp(currency, m->key) == 0) {
            strncpy(mapped_currency, m->value, sizeof(mapped_currency) - 1);
            mapped_currency[sizeof(mapped_currency) - 1] = '\0';
            break;
        }
    }

    json_t *root = json_object();
    json_object_set_new(root, "timestamp", json_string(timestamp));
    json_object_set_new(root, "exchange", json_string(exchange));
    json_object_set_new(root, "currency", json_string(mapped_currency));
    json_object_set_new(root, "price", json_string(price));

    char *json_str = json_dumps(root, JSON_INDENT(4));

    fprintf(data_file, "%s,\n", json_str);
    
    free(json_str);
    json_decref(root);
    fflush(data_file);
}
 
 /* Unified Callback for all exchanges */
 static int callback_combined(struct lws *wsi, enum lws_callback_reasons reason,
                               void *user, void *in, size_t len) {
     const struct lws_protocols *protocol_struct = lws_get_protocol(wsi);
     const char *protocol = (protocol_struct) ? protocol_struct->name : "unknown";
     
     switch (reason) {
         case LWS_CALLBACK_CLIENT_ESTABLISHED:
             if (strcmp(protocol, "binance-websocket") == 0) {
                 printf("[INFO] Binance WebSocket Connection Established!\n");
                 const char *subscribe_msg =
                     "{\"method\": \"SUBSCRIBE\", \"params\": ["
                     "\"btcusdt@ticker\", "
                     "\"ethusdt@ticker\", "
                     "\"adausdt@ticker\""
                     "], \"id\": 1}";
                 size_t msg_len = strlen(subscribe_msg);
                 unsigned char *buf = malloc(LWS_PRE + msg_len);
                 if (!buf) {
                     printf("[ERROR] Memory allocation failed for Binance message\n");
                     return -1;
                 }
                 memcpy(buf + LWS_PRE, subscribe_msg, msg_len);
                 int bytes_sent = lws_write(wsi, buf + LWS_PRE, msg_len, LWS_WRITE_TEXT);
                 if (bytes_sent < 0)
                     printf("[ERROR] Failed to send Binance subscription message\n");
                 else
                     printf("[INFO] Sent subscription message to Binance\n");
                 free(buf);
             }
             else if (strcmp(protocol, "coinbase-websocket") == 0) {
                 printf("[INFO] Coinbase WebSocket Connection Established!\n");
                 const char *subscribe_msg =
                     "{\"type\": \"subscribe\", \"channels\": ["
                     "{ \"name\": \"ticker\", \"product_ids\": [\"BTC-USD\", \"ETH-USD\", \"ADA-USD\"] }"
                     "]}";
                 size_t msg_len = strlen(subscribe_msg);
                 unsigned char *buf = malloc(LWS_PRE + msg_len);
                 if (!buf) {
                     printf("[ERROR] Memory allocation failed for Coinbase message\n");
                     return -1;
                 }
                 memcpy(buf + LWS_PRE, subscribe_msg, msg_len);
                 int bytes_sent = lws_write(wsi, buf + LWS_PRE, msg_len, LWS_WRITE_TEXT);
                 if (bytes_sent < 0)
                     printf("[ERROR] Failed to send Coinbase subscription message\n");
                 else
                     printf("[INFO] Sent subscription message to Coinbase\n");
                 free(buf);
             }
             else if (strcmp(protocol, "kraken-websocket") == 0) {
                 printf("[INFO] Kraken WebSocket Connection Established!\n");
                 const char *subscribe_msg =
                     "{\"event\": \"subscribe\", \"pair\": [\"XBT/USD\",\"ETH/USD\",\"ADA/USD\"], \"subscription\": {\"name\": \"ticker\"}}";
                 size_t msg_len = strlen(subscribe_msg);
                 unsigned char *buf = malloc(LWS_PRE + msg_len);
                 if (!buf) {
                     printf("[ERROR] Memory allocation failed for Kraken message\n");
                     return -1;
                 }
                 memcpy(buf + LWS_PRE, subscribe_msg, msg_len);
                 int bytes_sent = lws_write(wsi, buf + LWS_PRE, msg_len, LWS_WRITE_TEXT);
                 if (bytes_sent < 0)
                     printf("[ERROR] Failed to send Kraken subscription message\n");
                 else
                     printf("[INFO] Sent subscription message to Kraken\n");
                 free(buf);
             }
             else if (strcmp(protocol, "bitfinex-websocket") == 0) {
                 printf("[INFO] Bitfinex WebSocket Connection Established!\n");
                 const char *subscribe_msg = "{\"event\": \"subscribe\", \"channel\": \"ticker\", \"symbol\": \"tBTCUSD\"}";
                 size_t msg_len = strlen(subscribe_msg);
                 unsigned char *buf = malloc(LWS_PRE + msg_len);
                 if (!buf) {
                     printf("[ERROR] Memory allocation failed for Bitfinex message\n");
                     return -1;
                 }
                 memcpy(buf + LWS_PRE, subscribe_msg, msg_len);
                 int bytes_sent = lws_write(wsi, buf + LWS_PRE, msg_len, LWS_WRITE_TEXT);
                 if (bytes_sent < 0)
                     printf("[ERROR] Failed to send Bitfinex subscription message\n");
                 else
                     printf("[INFO] Sent subscription message to Bitfinex\n");
                 free(buf);
             }
             else if (strcmp(protocol, "huobi-websocket") == 0) {
                 printf("[INFO] Huobi WebSocket Connection Established!\n");
                 const char *subscribe_msg = "{\"sub\": \"market.btcusdt.ticker\", \"id\": \"huobi_ticker\"}";
                 size_t msg_len = strlen(subscribe_msg);
                 unsigned char *buf = malloc(LWS_PRE + msg_len);
                 if (!buf) {
                     printf("[ERROR] Memory allocation failed for Huobi message\n");
                     return -1;
                 }
                 memcpy(buf + LWS_PRE, subscribe_msg, msg_len);
                 int bytes_sent = lws_write(wsi, buf + LWS_PRE, msg_len, LWS_WRITE_TEXT);
                 if (bytes_sent < 0)
                     printf("[ERROR] Failed to send Huobi subscription message\n");
                 else
                     printf("[INFO] Sent subscription message to Huobi\n");
                 free(buf);
             }
             else if (strcmp(protocol, "okx-websocket") == 0) {
                 printf("[INFO] OKX WebSocket Connection Established!\n");
                 const char *subscribe_msg = "{\"op\": \"subscribe\", \"args\": [\"spot/ticker:BTC-USDT\"]}";
                 size_t msg_len = strlen(subscribe_msg);
                 unsigned char *buf = malloc(LWS_PRE + msg_len);
                 if (!buf) {
                     printf("[ERROR] Memory allocation failed for OKX message\n");
                     return -1;
                 }
                 memcpy(buf + LWS_PRE, subscribe_msg, msg_len);
                 int bytes_sent = lws_write(wsi, buf + LWS_PRE, msg_len, LWS_WRITE_TEXT);
                 if (bytes_sent < 0)
                     printf("[ERROR] Failed to send OKX subscription message\n");
                 else
                     printf("[INFO] Sent subscription message to OKX\n");
                 free(buf);
             }
             break;
             
         case LWS_CALLBACK_CLIENT_RECEIVE:
             if (strcmp(protocol, "binance-websocket") == 0) {
                 printf("[DATA][Binance] %.*s\n", (int)len, (char *)in);
                 char price[32] = {0}, currency[32] = {0}, time_ms[32] = {0}, timestamp[64] = {0};
                 if (extract_price((char *)in, "\"E\":", time_ms, sizeof(time_ms)) &&
                     extract_price((char *)in, "\"s\":\"", currency, sizeof(currency)) &&
                     extract_price((char *)in, "\"c\":\"", price, sizeof(price))) {
                     convert_binance_timestamp(timestamp, sizeof(timestamp), time_ms);
                     log_price(timestamp, "Binance", currency, price);
                 }
             }
             else if (strcmp(protocol, "coinbase-websocket") == 0) {
                 printf("[DATA][Coinbase] %.*s\n", (int)len, (char *)in);
                 char price[32] = {0}, currency[32] = {0}, timestamp[64] = {0};
                 if (extract_price((char *)in, "\"time\":\"", timestamp, sizeof(timestamp)) &&
                     extract_price((char *)in, "\"product_id\":\"", currency, sizeof(currency)) &&
                     extract_price((char *)in, "\"price\":\"", price, sizeof(price))) {
                     log_price(timestamp, "Coinbase", currency, price);
                 }
             }
             else if (strcmp(protocol, "kraken-websocket") == 0) {
                 printf("[DATA][Kraken] %.*s\n", (int)len, (char *)in);
                 char price[32] = {0}, currency[32] = {0}, timestamp[32] = {0};
                 if (extract_price((char *)in, "\"c\":[\"", price, sizeof(price))) {
                     if (!extract_price((char *)in, "\",\"ticker\",\"", currency, sizeof(currency)))
                         strncpy(currency, "unknown", sizeof(currency));
                     get_timestamp(timestamp, sizeof(timestamp));
                     log_price(timestamp, "Kraken", currency, price);
                 }
             }
             else if (strcmp(protocol, "bitfinex-websocket") == 0) {
                 printf("[DATA][Bitfinex] %.*s\n", (int)len, (char *)in);
                 char price[32] = {0}, timestamp[32] = {0};
                 if (extract_bitfinex_price((char *)in, price, sizeof(price))) {
                     get_timestamp(timestamp, sizeof(timestamp));
                     log_price(timestamp, "Bitfinex", "tBTCUSD", price);
                 }
             }
             else if (strcmp(protocol, "huobi-websocket") == 0) {
                 printf("[DATA][Huobi] %.*s\n", (int)len, (char *)in);
                 char price[32] = {0}, currency[32] = {0}, timestamp[64] = {0};
                 if (extract_numeric((char *)in, "\"close\":", price, sizeof(price))) {
                     extract_huobi_currency((char *)in, currency, sizeof(currency));
                     char ts_str[32] = {0};
                     if (extract_numeric((char *)in, "\"ts\":", ts_str, sizeof(ts_str)))
                         convert_binance_timestamp(timestamp, sizeof(timestamp), ts_str);
                     else
                         get_timestamp(timestamp, sizeof(timestamp));
                     log_price(timestamp, "Huobi", currency, price);
                 }
             }
             else if (strcmp(protocol, "okx-websocket") == 0) {
                 printf("[DATA][OKX] %.*s\n", (int)len, (char *)in);
                 char price[32] = {0}, currency[32] = {0}, timestamp[64] = {0};
                 if (extract_price((char *)in, "\"last\":\"", price, sizeof(price)) &&
                     extract_price((char *)in, "\"instId\":\"", currency, sizeof(currency))) {
                     if (!extract_price((char *)in, "\"ts\":\"", timestamp, sizeof(timestamp)))
                         get_timestamp(timestamp, sizeof(timestamp));
                     log_price(timestamp, "OKX", currency, price);
                 }
             }
             break;
             
         case LWS_CALLBACK_CLIENT_CLOSED:
             printf("[INFO] %s WebSocket Connection Closed.\n", protocol);
             break;
             
         case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
             printf("[ERROR] %s WebSocket Connection Error!\n", protocol);
             break;
             
         default:
             break;
     }
     
     return 0;
 }
 
 static struct lws_protocols protocols[] = {
     { "binance-websocket", callback_combined, 0, 4096 },
     { "coinbase-websocket", callback_combined, 0, 4096 },
     { "kraken-websocket", callback_combined, 0, 4096 },
     { "bitfinex-websocket", callback_combined, 0, 4096 },
     { "huobi-websocket", callback_combined, 0, 4096 },
     { "okx-websocket", callback_combined, 0, 4096 },
     { NULL, NULL, 0, 0 }
 };
 
 int compare_entries_csv(const void *a, const void *b) {
     const EntryCSV *entry_a = (const EntryCSV *)a;
     const EntryCSV *entry_b = (const EntryCSV *)b;
     return strcmp(entry_a->timestamp, entry_b->timestamp);
 }
 
 void free_entry_csv(EntryCSV *entry) {
     free(entry->timestamp);
     free(entry->exchange);
     free(entry->product);
 }
 
 EntryCSV *parse_line_csv(char *line) {
     char *timestamp_start = strchr(line, '[');
     if (!timestamp_start) return NULL;
     timestamp_start++;
     char *timestamp_end = strchr(timestamp_start, ']');
     if (!timestamp_end) return NULL;
     size_t ts_len = timestamp_end - timestamp_start;
     char *timestamp = malloc(ts_len + 1);
     strncpy(timestamp, timestamp_start, ts_len);
     timestamp[ts_len] = '\0';
 
     char *exchange_start = timestamp_end + 2;
     if (*exchange_start != '[') {
         free(timestamp);
         return NULL;
     }
     exchange_start++;
     char *exchange_end = strchr(exchange_start, ']');
     if (!exchange_end) {
         free(timestamp);
         return NULL;
     }
     size_t exch_len = exchange_end - exchange_start;
     char *exchange = malloc(exch_len + 1);
     strncpy(exchange, exchange_start, exch_len);
     exchange[exch_len] = '\0';
 
     char *product_start = exchange_end + 2;
     if (*product_start != '[') {
         free(timestamp);
         free(exchange);
         return NULL;
     }
     product_start++;
     char *product_end = strchr(product_start, ']');
     if (!product_end) {
         free(timestamp);
         free(exchange);
         return NULL;
     }
     size_t prod_len = product_end - product_start;
     char *product = malloc(prod_len + 1);
     strncpy(product, product_start, prod_len);
     product[prod_len] = '\0';
 
     char *price_start = strstr(product_end, "Price: ");
     if (!price_start) {
         free(timestamp);
         free(exchange);
         free(product);
         return NULL;
     }
     price_start += 7;
     double price = strtod(price_start, NULL);
 
     EntryCSV *entry = malloc(sizeof(EntryCSV));
     entry->timestamp = timestamp;
     entry->exchange = exchange;
     entry->product = product;
     entry->price = price;
     return entry;
 }
 
 void process_data_and_write_csv(EntryCSV *entries, size_t num_entries, const char *output_file) {
     qsort(entries, num_entries, sizeof(EntryCSV), compare_entries_csv);
 
     for (size_t i = 0; i < num_entries; i++) {
        EntryCSV *entry = &entries[i];
    
        for (ProductMapping *m = product_mappings_arr; m->key; m++) {
            if (strcmp(entry->product, m->key) == 0) {
                free(entry->product);
                entry->product = strdup(m->value);
                break;
            }
        }
    
        for (int j = 0; j < 3; j++) {
            if (strcmp(entry->product, price_counters_arr[j].product) == 0) {
                price_counters_arr[j].value = entry->price;
                price_counters_arr[j].initialized = 1;
                break;
            }
        }
    
        if (strcmp(entry->product, "unknown") == 0) {
            double min_diff = INFINITY;
            int closest = -1;
            for (int j = 0; j < 3; j++) {
                if (price_counters_arr[j].initialized) {
                    double diff = fabs(entry->price - price_counters_arr[j].value);
                    if (diff < min_diff) {
                        min_diff = diff;
                        closest = j;
                    }
                }
            }
            if (closest != -1) {
                free(entry->product);
                entry->product = strdup(price_counters_arr[closest].product);
            }
        }
    }    
 
     FILE *output = fopen(output_file, "w");
     if (!output) {
         perror("Error opening output file");
         return;
     }
     fprintf(output, "index,time,exchange,product,price\n");
     for (size_t i = 0; i < num_entries; i++) {
         EntryCSV *entry = &entries[i];
         fprintf(output, "%zu,%s,%s,%s,%.8f\n", i + 1, entry->timestamp, entry->exchange, entry->product, entry->price);
     }
     fclose(output);
 }
 
 void run_csv_processing_mode(const char *input_file, const char *output_file) {
     FILE *input = fopen(input_file, "r");
     if (!input) {
         perror("Error opening input file");
         exit(1);
     }
     EntryCSV *entries = NULL;
     size_t num_entries = 0, capacity = 0;
     char *line = NULL;
     size_t line_len = 0;
     while (getline(&line, &line_len, input) != -1) {
         char *trimmed_line = line;
         while (*trimmed_line == ' ' || *trimmed_line == '\t') trimmed_line++;
         size_t len = strlen(trimmed_line);
         while (len > 0 && (trimmed_line[len-1] == '\n' || trimmed_line[len-1] == '\r'))
             trimmed_line[--len] = '\0';
         if (len == 0) continue;
         EntryCSV *entry = parse_line_csv(trimmed_line);
         if (!entry) {
             fprintf(stderr, "Skipping invalid line: %s\n", trimmed_line);
             continue;
         }
         if (num_entries >= capacity) {
             capacity = capacity ? capacity * 2 : 16;
             entries = realloc(entries, capacity * sizeof(EntryCSV));
             if (!entries) {
                 perror("Memory allocation failed");
                 exit(1);
             }
         }
         entries[num_entries++] = *entry;
         free(entry);
     }
     free(line);
     fclose(input);
     process_data_and_write_csv(entries, num_entries, output_file);
     for (size_t i = 0; i < num_entries; i++) {
         free_entry_csv(&entries[i]);
     }
     free(entries);
 }
 
 int main(int argc, char *argv[]) {
     if (argc == 3) {
         run_csv_processing_mode(argv[1], argv[2]);
         return 0;
     }
     
     struct lws_context *context;
     struct lws_context_creation_info context_info;
     struct lws_client_connect_info binance_connect_info, coinbase_connect_info;
     struct lws_client_connect_info kraken_connect_info, bitfinex_connect_info;
     struct lws_client_connect_info huobi_connect_info, okx_connect_info;
 
     memset(&context_info, 0, sizeof(context_info));
     context_info.port = CONTEXT_PORT_NO_LISTEN;
     context_info.protocols = protocols;
     context_info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
     context = lws_create_context(&context_info);
     if (!context) {
         printf("[ERROR] Failed to create WebSocket context\n");
         return -1;
     }
 
     data_file = fopen("output_data.json", "a");
     if (!data_file) {
         printf("[ERROR] Failed to open log file\n");
         lws_context_destroy(context);
         return -1;
     }
 
     /* Binance Connection */
     memset(&binance_connect_info, 0, sizeof(binance_connect_info));
     binance_connect_info.context   = context;
     binance_connect_info.address   = "stream.binance.us";
     binance_connect_info.port      = 9443;
     binance_connect_info.path      = "/stream?streams=btcusdt@trade/btcusdt@ticker/ethusdt@trade/ethusdt@ticker/adausdt@trade/adausdt@ticker";
     binance_connect_info.host      = "stream.binance.us";
     binance_connect_info.origin    = "stream.binance.us";
     binance_connect_info.protocol  = protocols[0].name;
     binance_connect_info.ssl_connection = LCCSCF_USE_SSL;
     if (!lws_client_connect_via_info(&binance_connect_info))
         printf("[ERROR] Failed to connect to Binance WebSocket server\n");
 
     /* Coinbase Connection */
     memset(&coinbase_connect_info, 0, sizeof(coinbase_connect_info));
     coinbase_connect_info.context   = context;
     coinbase_connect_info.address   = "ws-feed.exchange.coinbase.com";
     coinbase_connect_info.port      = 443;
     coinbase_connect_info.path      = "/";
     coinbase_connect_info.host      = "ws-feed.exchange.coinbase.com";
     coinbase_connect_info.origin    = "ws-feed.exchange.coinbase.com";
     coinbase_connect_info.protocol  = protocols[1].name;
     coinbase_connect_info.ssl_connection = LCCSCF_USE_SSL;
     if (!lws_client_connect_via_info(&coinbase_connect_info))
         printf("[ERROR] Failed to connect to Coinbase WebSocket server\n");
 
     /* Kraken Connection */
     memset(&kraken_connect_info, 0, sizeof(kraken_connect_info));
     kraken_connect_info.context   = context;
     kraken_connect_info.address   = "ws.kraken.com";
     kraken_connect_info.port      = 443;
     kraken_connect_info.path      = "/";
     kraken_connect_info.host      = "ws.kraken.com";
     kraken_connect_info.origin    = "ws.kraken.com";
     kraken_connect_info.protocol  = protocols[2].name;
     kraken_connect_info.ssl_connection = LCCSCF_USE_SSL;
     if (!lws_client_connect_via_info(&kraken_connect_info))
         printf("[ERROR] Failed to connect to Kraken WebSocket server\n");
 
     /* Bitfinex Connection */
     memset(&bitfinex_connect_info, 0, sizeof(bitfinex_connect_info));
     bitfinex_connect_info.context   = context;
     bitfinex_connect_info.address   = "api-pub.bitfinex.com";
     bitfinex_connect_info.port      = 443;
     bitfinex_connect_info.path      = "/ws/2";
     bitfinex_connect_info.host      = "api-pub.bitfinex.com";
     bitfinex_connect_info.origin    = "api-pub.bitfinex.com";
     bitfinex_connect_info.protocol  = protocols[3].name;
     bitfinex_connect_info.ssl_connection = LCCSCF_USE_SSL;
     if (!lws_client_connect_via_info(&bitfinex_connect_info))
         printf("[ERROR] Failed to connect to Bitfinex WebSocket server\n");
 
     /* Huobi Connection */
     memset(&huobi_connect_info, 0, sizeof(huobi_connect_info));
     huobi_connect_info.context   = context;
     huobi_connect_info.address   = "api.huobi.pro";
     huobi_connect_info.port      = 443;
     huobi_connect_info.path      = "/ws";
     huobi_connect_info.host      = "api.huobi.pro";
     huobi_connect_info.origin    = "api.huobi.pro";
     huobi_connect_info.protocol  = protocols[4].name;
     huobi_connect_info.ssl_connection = LCCSCF_USE_SSL;
     if (!lws_client_connect_via_info(&huobi_connect_info))
         printf("[ERROR] Failed to connect to Huobi WebSocket server\n");
 
     /* OKX Connection */
     memset(&okx_connect_info, 0, sizeof(okx_connect_info));
     okx_connect_info.context   = context;
     okx_connect_info.address   = "ws.okx.com";
     okx_connect_info.port      = 8443;
     okx_connect_info.path      = "/ws/v5/public";
     okx_connect_info.host      = "ws.okx.com";
     okx_connect_info.origin    = "ws.okx.com";
     okx_connect_info.protocol  = protocols[5].name;
     okx_connect_info.ssl_connection = LCCSCF_USE_SSL;
     if (!lws_client_connect_via_info(&okx_connect_info))
         printf("[ERROR] Failed to connect to OKX WebSocket server\n");
 
     while (lws_service(context, 1000) >= 0) {
         /* Event loop */
     }
 
     printf("[INFO] Cleaning up WebSocket context...\n");
     fclose(data_file);
     lws_context_destroy(context);
     return 0;
 }
