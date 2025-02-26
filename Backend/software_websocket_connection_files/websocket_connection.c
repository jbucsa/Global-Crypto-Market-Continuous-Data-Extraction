/*
 * Crypto Exchange WebSocket Logger
 * 
 * This program connects to multiple cryptocurrency exchanges via WebSocket APIs
 * to receive real-time market data, extract relevant information, and log the
 * prices along with timestamps to a file.
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
 *  - libwebsockets
 *  - Standard C libraries (stdio, stdlib, string, time, sys/time)
 * 
 * Usage:
 *  - Compile using a C compiler with the required dependencies.
 *  - Run the executable to establish WebSocket connections and start logging.
 * 
 * Date: 2/26/2025
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <libwebsockets.h>

FILE *data_file = NULL;

/* Convert Binance/any millisecond timestamp to ISO 8601 format */
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


/* Extract a value (quotes) from JSON using key */
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
    while (*pos && ((*pos >= '0' && *pos <= '9') || *pos == '.' || *pos == '-') && i < dest_size - 1) {
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

/* Log price with provided timestamp, exchange, and currency */
static void log_price(const char *timestamp, const char *exchange, const char *currency, const char *price) {
    if (!data_file)
        return;
    fprintf(data_file, "[%s][%s][%s] Price: %s\n", timestamp, exchange, currency, price);
    fflush(data_file);
}

/* Binance Callback */
static int callback_binance(struct lws *wsi, enum lws_callback_reasons reason,
                            void *user, void *in, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            printf("[INFO] Binance WebSocket Connection Established!\n");
            {
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
            break;
        case LWS_CALLBACK_CLIENT_RECEIVE: {
            printf("[DATA][Binance] %.*s\n", (int)len, (char *)in);
            char price[32] = {0}, currency[32] = {0}, time_ms[32] = {0}, timestamp[64] = {0};
            if (extract_price((char *)in, "\"E\":", time_ms, sizeof(time_ms)) &&
                extract_price((char *)in, "\"s\":\"", currency, sizeof(currency)) &&
                extract_price((char *)in, "\"c\":\"", price, sizeof(price))) {
                convert_binance_timestamp(timestamp, sizeof(timestamp), time_ms);
                log_price(timestamp, "Binance", currency, price);
            }
            break;
        }
        case LWS_CALLBACK_CLIENT_CLOSED:
            printf("[INFO] Binance WebSocket Connection Closed.\n");
            break;
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            printf("[ERROR] Binance WebSocket Connection Error!\n");
            break;
        default:
            break;
    }
    return 0;
}

/* Coinbase Callback */
static int callback_coinbase(struct lws *wsi, enum lws_callback_reasons reason,
                             void *user, void *in, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            printf("[INFO] Coinbase WebSocket Connection Established!\n");
            {
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
            break;
        case LWS_CALLBACK_CLIENT_RECEIVE: {
            printf("[DATA][Coinbase] %.*s\n", (int)len, (char *)in);
            char price[32] = {0}, currency[32] = {0}, timestamp[64] = {0};
            if (extract_price((char *)in, "\"time\":\"", timestamp, sizeof(timestamp)) &&
                extract_price((char *)in, "\"product_id\":\"", currency, sizeof(currency)) &&
                extract_price((char *)in, "\"price\":\"", price, sizeof(price))) {
                log_price(timestamp, "Coinbase", currency, price);
            }
            break;
        }
        case LWS_CALLBACK_CLIENT_CLOSED:
            printf("[INFO] Coinbase WebSocket Connection Closed.\n");
            break;
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            printf("[ERROR] Coinbase WebSocket Connection Error!\n");
            break;
        default:
            break;
    }
    return 0;
}

/* Kraken Callback */
static int callback_kraken(struct lws *wsi, enum lws_callback_reasons reason,
                           void *user, void *in, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            printf("[INFO] Kraken WebSocket Connection Established!\n");
            {
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
            break;
        case LWS_CALLBACK_CLIENT_RECEIVE: {
            printf("[DATA][Kraken] %.*s\n", (int)len, (char *)in);
            char price[32] = {0}, currency[32] = {0}, timestamp[32] = {0};
            if (extract_price((char *)in, "\"c\":[\"", price, sizeof(price))) {
                if (!extract_price((char *)in, "\",\"ticker\",\"", currency, sizeof(currency)))
                    strncpy(currency, "unknown", sizeof(currency));
                get_timestamp(timestamp, sizeof(timestamp));
                log_price(timestamp, "Kraken", currency, price);
            }
            break;
        }
        case LWS_CALLBACK_CLIENT_CLOSED:
            printf("[INFO] Kraken WebSocket Connection Closed.\n");
            break;
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            printf("[ERROR] Kraken WebSocket Connection Error!\n");
            break;
        default:
            break;
    }
    return 0;
}

/* Bitfinex Callback */
static int callback_bitfinex(struct lws *wsi, enum lws_callback_reasons reason,
                             void *user, void *in, size_t len) {
    switch(reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            printf("[INFO] Bitfinex WebSocket Connection Established!\n");
            {
                const char *subscribe_msg = "{\"event\": \"subscribe\", \"channel\": \"ticker\", \"symbol\": \"tBTCUSD\"}";
                size_t msg_len = strlen(subscribe_msg);
                unsigned char *buf = malloc(LWS_PRE + msg_len);
                if(!buf) {
                    printf("[ERROR] Memory allocation failed for Bitfinex message\n");
                    return -1;
                }
                memcpy(buf + LWS_PRE, subscribe_msg, msg_len);
                int bytes_sent = lws_write(wsi, buf + LWS_PRE, msg_len, LWS_WRITE_TEXT);
                if(bytes_sent < 0)
                    printf("[ERROR] Failed to send Bitfinex subscription message\n");
                else
                    printf("[INFO] Sent subscription message to Bitfinex\n");
                free(buf);
            }
            break;
        case LWS_CALLBACK_CLIENT_RECEIVE: {
            printf("[DATA][Bitfinex] %.*s\n", (int)len, (char *)in);
            char price[32] = {0}, timestamp[32] = {0};
            if (extract_bitfinex_price((char *)in, price, sizeof(price))) {
                get_timestamp(timestamp, sizeof(timestamp));
                log_price(timestamp, "Bitfinex", "tBTCUSD", price);
            }
            break;
        }
        case LWS_CALLBACK_CLIENT_CLOSED:
            printf("[INFO] Bitfinex WebSocket Connection Closed.\n");
            break;
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            printf("[ERROR] Bitfinex WebSocket Connection Error!\n");
            break;
        default:
            break;
    }
    return 0;
}

/* Huobi Callback */
static int callback_huobi(struct lws *wsi, enum lws_callback_reasons reason,
                          void *user, void *in, size_t len) {
    switch(reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            printf("[INFO] Huobi WebSocket Connection Established!\n");
            {
                const char *subscribe_msg = "{\"sub\": \"market.btcusdt.ticker\", \"id\": \"huobi_ticker\"}";
                size_t msg_len = strlen(subscribe_msg);
                unsigned char *buf = malloc(LWS_PRE + msg_len);
                if(!buf) {
                    printf("[ERROR] Memory allocation failed for Huobi message\n");
                    return -1;
                }
                memcpy(buf + LWS_PRE, subscribe_msg, msg_len);
                int bytes_sent = lws_write(wsi, buf + LWS_PRE, msg_len, LWS_WRITE_TEXT);
                if(bytes_sent < 0)
                    printf("[ERROR] Failed to send Huobi subscription message\n");
                else
                    printf("[INFO] Sent subscription message to Huobi\n");
                free(buf);
            }
            break;
        case LWS_CALLBACK_CLIENT_RECEIVE: {
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
            break;
        }
        case LWS_CALLBACK_CLIENT_CLOSED:
            printf("[INFO] Huobi WebSocket Connection Closed.\n");
            break;
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            printf("[ERROR] Huobi WebSocket Connection Error!\n");
            break;
        default:
            break;
    }
    return 0;
}

/* OKX Callback */
static int callback_okx(struct lws *wsi, enum lws_callback_reasons reason,
                        void *user, void *in, size_t len) {
    switch(reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            printf("[INFO] OKX WebSocket Connection Established!\n");
            {
                const char *subscribe_msg = "{\"op\": \"subscribe\", \"args\": [\"spot/ticker:BTC-USDT\"]}";
                size_t msg_len = strlen(subscribe_msg);
                unsigned char *buf = malloc(LWS_PRE + msg_len);
                if(!buf) {
                    printf("[ERROR] Memory allocation failed for OKX message\n");
                    return -1;
                }
                memcpy(buf + LWS_PRE, subscribe_msg, msg_len);
                int bytes_sent = lws_write(wsi, buf + LWS_PRE, msg_len, LWS_WRITE_TEXT);
                if(bytes_sent < 0)
                    printf("[ERROR] Failed to send OKX subscription message\n");
                else
                    printf("[INFO] Sent subscription message to OKX\n");
                free(buf);
            }
            break;
        case LWS_CALLBACK_CLIENT_RECEIVE: {
            printf("[DATA][OKX] %.*s\n", (int)len, (char *)in);
            char price[32] = {0}, currency[32] = {0}, timestamp[64] = {0};
            if (extract_price((char *)in, "\"last\":\"", price, sizeof(price)) &&
                extract_price((char *)in, "\"instId\":\"", currency, sizeof(currency))) {
                if (!extract_price((char *)in, "\"ts\":\"", timestamp, sizeof(timestamp)))
                    get_timestamp(timestamp, sizeof(timestamp));
                log_price(timestamp, "OKX", currency, price);
            }
            break;
        }
        case LWS_CALLBACK_CLIENT_CLOSED:
            printf("[INFO] OKX WebSocket Connection Closed.\n");
            break;
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            printf("[ERROR] OKX WebSocket Connection Error!\n");
            break;
        default:
            break;
    }
    return 0;
}

static struct lws_protocols protocols[] = {
    { "binance-websocket", callback_binance, 0, 4096 },
    { "coinbase-websocket", callback_coinbase, 0, 4096 },
    { "kraken-websocket", callback_kraken, 0, 4096 },
    { "bitfinex-websocket", callback_bitfinex, 0, 4096 },
    { "huobi-websocket", callback_huobi, 0, 4096 },
    { "okx-websocket", callback_okx, 0, 4096 },
    { NULL, NULL, 0, 0 }
};

int main() {
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

    data_file = fopen("output_data.txt", "a");
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
