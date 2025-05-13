/*
 * Exchange Product ID Fetcher
 * 
 * This utility gathers available trading pairs from multiple crypto exchange REST APIs
 * and outputs product IDs in WebSocket-compatible JSON formats for both tickers and trades.
 * 
 * Features:
 *  - Fetches product data via REST API endpoints.
 *  - Writes formatted symbol lists to JSON-style .txt files.
 *  - Outputs chunked or full listings depending on exchange (e.g., Huobi).
 *  - Handles both ticker and trade subscription formats (e.g., OKX, Binance).
 * 
 * Dependencies:
 *  - libcurl: HTTP client for API requests.
 *  - jansson: JSON parsing and generation.
 *  - Standard C libraries (stdio, stdlib, string).
 * 
 * Build & Run:
 *  sudo apt install libcurl4-openssl-dev libjansson-dev build-essential
 *  dos2unix fetch_currency_id.c
 *  gcc fetch_currency_id.c -o fetch_currency_id -lcurl -ljansson
 *  ./fetch_currency_id
 * 
 * Output Directory:
 *  - Writes all exchange product ID files to `currency_text_files/`.
 * 
 * Created: 4/29/2025
 * Updated: 5/8/2025
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <ctype.h>
 #include <curl/curl.h>
 #include <jansson.h>

 
 struct MemoryStruct {
     char *memory;
     size_t size;
 };
 
 static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
     size_t realsize = size * nmemb;
     struct MemoryStruct *mem = (struct MemoryStruct *)userp;
 
     char *ptr = realloc(mem->memory, mem->size + realsize + 1);
     if (!ptr) {
         fprintf(stderr, "Not enough memory\n");
         return 0;
     }
 
     mem->memory = ptr;
     memcpy(&(mem->memory[mem->size]), contents, realsize);
     mem->size += realsize;
     mem->memory[mem->size] = '\0';
 
     return realsize;
 }
 
 void fetch_coinbase_product_ids() {
     CURL *curl;
     CURLcode res;
 
     struct MemoryStruct chunk = {0};
     chunk.memory = malloc(1);
     chunk.size = 0;
 
     curl = curl_easy_init();
     if (curl) {
         curl_easy_setopt(curl, CURLOPT_URL, "https://api.exchange.coinbase.com/products");
         curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
         curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
         curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
         res = curl_easy_perform(curl);
 
         if (res == CURLE_OK) {
             json_error_t error;
             json_t *root = json_loads(chunk.memory, 0, &error);
             if (root && json_is_array(root)) {
                 FILE *fp = fopen("currency_text_files/coinbase_currency_ids.txt", "w");
                 if (!fp) {
                     fprintf(stderr, "Failed to open output file\n");
                     json_decref(root);
                     return;
                 }
 
                 fprintf(fp, "[");
                 for (size_t i = 0; i < json_array_size(root); i++) {
                     json_t *product = json_array_get(root, i);
                     json_t *id_val = json_object_get(product, "id");
                     if (json_is_string(id_val)) {
                         const char *id = json_string_value(id_val);
                         fprintf(fp, "\"%s\"", id);
                         if (i < json_array_size(root) - 1) {
                             fprintf(fp, ", ");
                         }
                     }
                 }
                 fprintf(fp, "]\n");
                 fclose(fp);
 
                 printf("Coinbase Product IDs saved to coinbase_currency_ids.txt\n");
                 json_decref(root);
             } else {
                 fprintf(stderr, "JSON parse error: %s\n", error.text);
             }
         } else {
             fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
         }
 
         curl_easy_cleanup(curl);
         free(chunk.memory);
     }
 }

 void fetch_huobi_product_ids() {
    CURL *curl;
    CURLcode res;

    struct MemoryStruct chunk = {0};
    chunk.memory = malloc(1);
    chunk.size = 0;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.huobi.pro/v1/common/symbols");
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        res = curl_easy_perform(curl);

        if (res == CURLE_OK) {
            json_error_t error;
            json_t *root = json_loads(chunk.memory, 0, &error);
            json_t *data = json_object_get(root, "data");

            if (data && json_is_array(data)) {
                size_t total = json_array_size(data);
                size_t chunk_size = 100;
                size_t chunk_index = 0;

                for (size_t i = 0; i < total; i += chunk_size) {
                    char filename[64];
                    snprintf(filename, sizeof(filename), "currency_text_files/huobi_currency_chunk_%zu.txt", chunk_index++);
                    FILE *fp = fopen(filename, "w");
                    if (!fp) {
                        fprintf(stderr, "[ERROR] Could not open %s for writing\n", filename);
                        continue;
                    }

                    fprintf(fp, "[");
                    size_t written = 0;

                    for (size_t j = i; j < i + chunk_size && j < total; j++) {
                        json_t *item = json_array_get(data, j);
                        const char *base = json_string_value(json_object_get(item, "base-currency"));
                        const char *quote = json_string_value(json_object_get(item, "quote-currency"));
                        if (base && quote) {
                            if (written > 0)
                                fprintf(fp, ", ");
                            fprintf(fp, "\"%s%s\"", base, quote);
                            written++;
                        }
                    }

                    fprintf(fp, "]\n");
                    fclose(fp);
                    // printf("[INFO] Wrote %zu symbols to %s\n", written, filename);
                }
                printf("Huobi Product IDs saved to huobi_currency_ids_XX.txt\n");
                json_decref(root);
            } else {
                fprintf(stderr, "[ERROR] Invalid or missing 'data' array\n");
            }
        } else {
            fprintf(stderr, "[ERROR] curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        curl_easy_cleanup(curl);
        free(chunk.memory);
    }
}

void fetch_huobi_product_ids_full() {
    CURL *curl;
    CURLcode res;

    struct MemoryStruct chunk = {0};
    chunk.memory = malloc(1);
    chunk.size = 0;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.huobi.pro/v1/common/symbols");
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        res = curl_easy_perform(curl);

        if (res == CURLE_OK) {
            json_error_t error;
            json_t *root = json_loads(chunk.memory, 0, &error);
            json_t *data = json_object_get(root, "data");

            if (data && json_is_array(data)) {
                FILE *fp = fopen("currency_text_files/huobi_currency_ids.txt", "w");
                if (!fp) {
                    fprintf(stderr, "Failed to open output file\n");
                    json_decref(root);
                    return;
                }

                fprintf(fp, "[");
                for (size_t i = 0; i < json_array_size(data); i++) {
                    json_t *item = json_array_get(data, i);
                    const char *base = json_string_value(json_object_get(item, "base-currency"));
                    const char *quote = json_string_value(json_object_get(item, "quote-currency"));
                    if (base && quote) {
                        fprintf(fp, "\"%s%s\"", base, quote);
                        if (i < json_array_size(data) - 1) {
                            fprintf(fp, ", ");
                        }
                    }
                }
                fprintf(fp, "]\n");
                fclose(fp);

                printf("Huobi Product IDs saved to huobi_currency_ids.txt\n");
                json_decref(root);
            } else {
                fprintf(stderr, "Invalid or missing 'data' array\n");
            }
        } else {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        curl_easy_cleanup(curl);
        free(chunk.memory);
    }
}

void fetch_kraken_product_ids() {
    CURL *curl;
    CURLcode res;

    struct MemoryStruct chunk = {0};
    chunk.memory = malloc(1);
    chunk.size = 0;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.kraken.com/0/public/AssetPairs");
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        res = curl_easy_perform(curl);

        if (res == CURLE_OK) {
            json_error_t error;
            json_t *root = json_loads(chunk.memory, 0, &error);
            json_t *result = json_object_get(root, "result");

            if (result && json_is_object(result)) {
                FILE *fp = fopen("currency_text_files/kraken_currency_ids.txt", "w");
                if (!fp) {
                    fprintf(stderr, "Failed to open output file\n");
                    json_decref(root);
                    return;
                }

                fprintf(fp, "[");

                const char *key;
                json_t *value;
                int first = 1;

                json_object_foreach(result, key, value) {
                    const char *base = json_string_value(json_object_get(value, "base"));
                    const char *quote = json_string_value(json_object_get(value, "quote"));

                    if (base && quote) {
                        if (!first) {
                            fprintf(fp, ",");
                        }
                        fprintf(fp, "\"%s/%s\"", base, quote);
                        first = 0;
                    }
                }

                fprintf(fp, "]\n");
                fclose(fp);

                printf("Kraken Product IDs saved to kraken_currency_ids.txt\n");
                json_decref(root);
            } else {
                fprintf(stderr, "Invalid or missing 'result' object\n");
            }
        } else {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        curl_easy_cleanup(curl);
        free(chunk.memory);
    }
}


 void fetch_okx_product_ids_full() {
    CURL *curl;
    CURLcode res;

    struct MemoryStruct chunk = {0};
    chunk.memory = malloc(1);
    chunk.size = 0;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://www.okx.com/api/v5/public/instruments?instType=SPOT");
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        res = curl_easy_perform(curl);

        if (res == CURLE_OK) {
            json_error_t error;
            json_t *root = json_loads(chunk.memory, 0, &error);
            json_t *data = json_object_get(root, "data");

            if (data && json_is_array(data)) {
                FILE *fp = fopen("currency_text_files/okx_currency_ids.txt", "w");
                if (!fp) {
                    fprintf(stderr, "Failed to open output file\n");
                    json_decref(root);
                    return;
                }

                fprintf(fp, "[");
                for (size_t i = 0; i < json_array_size(data); i++) {
                    json_t *item = json_array_get(data, i);
                    const char *instId = json_string_value(json_object_get(item, "instId"));
                    if (instId) {
                        fprintf(fp, "{\"channel\": \"tickers\", \"instId\": \"%s\"}", instId);
                        if (i < json_array_size(data) - 1) {
                            fprintf(fp, ", ");
                        }
                    }
                }
                fprintf(fp, "]\n");
                fclose(fp);

                printf("OKX Product IDs saved to okx_currency_ids_trades.txt\n");
                json_decref(root);
            } else {
                fprintf(stderr, "Invalid or missing 'data' array\n");
            }
        } else {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        curl_easy_cleanup(curl);
        free(chunk.memory);
    }
}

void fetch_okx_product_ids() {
    CURL *curl;
    CURLcode res;

    struct MemoryStruct chunk = {0};
    chunk.memory = malloc(1);
    chunk.size = 0;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://www.okx.com/api/v5/public/instruments?instType=SPOT");
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        res = curl_easy_perform(curl);

        if (res == CURLE_OK) {
            json_error_t error;
            json_t *root = json_loads(chunk.memory, 0, &error);
            json_t *data = json_object_get(root, "data");

            if (data && json_is_array(data)) {
                size_t total = json_array_size(data);
                size_t chunk_size = 100;
                size_t chunk_index = 0;

                for (size_t i = 0; i < total; i += chunk_size) {
                    char filename[64];
                    snprintf(filename, sizeof(filename), "currency_text_files/okx_currency_chunk_%zu.txt", chunk_index++);
                    FILE *fp = fopen(filename, "w");
                    if (!fp) {
                        fprintf(stderr, "[ERROR] Could not open %s for writing\n", filename);
                        continue;
                    }

                    fprintf(fp, "[");
                    size_t written = 0;

                    for (size_t j = i; j < i + chunk_size && j < total; j++) {
                        json_t *item = json_array_get(data, j);
                        const char *instId = json_string_value(json_object_get(item, "instId"));
                        if (instId) {
                            if (written > 0)
                                fprintf(fp, ", ");
                            fprintf(fp, "{\"channel\": \"tickers\", \"instId\": \"%s\"}", instId);
                            written++;
                        }
                    }

                    fprintf(fp, "]\n");
                    fclose(fp);
                    // printf("[INFO] Wrote %zu instruments to %s\n", written, filename);
                }

                printf("OKX Product IDs saved to okx_currency_chunk_XX.txt\n");
                json_decref(root);
            } else {
                fprintf(stderr, "[ERROR] Invalid or missing 'data' array\n");
            }
        } else {
            fprintf(stderr, "[ERROR] curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        curl_easy_cleanup(curl);
        free(chunk.memory);
    }
}

void fetch_okx_product_ids_trades_full() {
    CURL *curl;
    CURLcode res;

    struct MemoryStruct chunk = {0};
    chunk.memory = malloc(1);
    chunk.size = 0;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://www.okx.com/api/v5/public/instruments?instType=SPOT");
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        res = curl_easy_perform(curl);

        if (res == CURLE_OK) {
            json_error_t error;
            json_t *root = json_loads(chunk.memory, 0, &error);
            json_t *data = json_object_get(root, "data");

            if (data && json_is_array(data)) {
                FILE *fp = fopen("currency_text_files/okx_currency_ids_trades.txt", "w");
                if (!fp) {
                    fprintf(stderr, "Failed to open output file\n");
                    json_decref(root);
                    return;
                }

                fprintf(fp, "[");
                for (size_t i = 0; i < json_array_size(data); i++) {
                    json_t *item = json_array_get(data, i);
                    const char *instId = json_string_value(json_object_get(item, "instId"));
                    if (instId) {
                        fprintf(fp, "{\"channel\": \"trades\", \"instId\": \"%s\"}", instId);
                        if (i < json_array_size(data) - 1) {
                            fprintf(fp, ", ");
                        }
                    }
                }
                fprintf(fp, "]\n");
                fclose(fp);

                printf("OKX Product IDs saved to okx_currency_ids.txt\n");
                json_decref(root);
            } else {
                fprintf(stderr, "Invalid or missing 'data' array\n");
            }
        } else {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        curl_easy_cleanup(curl);
        free(chunk.memory);
    }
}

void fetch_okx_product_ids_trades() {
    CURL *curl;
    CURLcode res;

    struct MemoryStruct chunk = {0};
    chunk.memory = malloc(1);
    chunk.size = 0;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://www.okx.com/api/v5/public/instruments?instType=SPOT");
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        res = curl_easy_perform(curl);

        if (res == CURLE_OK) {
            json_error_t error;
            json_t *root = json_loads(chunk.memory, 0, &error);
            json_t *data = json_object_get(root, "data");

            if (data && json_is_array(data)) {
                size_t total = json_array_size(data);
                size_t chunk_size = 100;
                size_t chunk_index = 0;

                for (size_t i = 0; i < total; i += chunk_size) {
                    char filename[64];
                    snprintf(filename, sizeof(filename), "currency_text_files/okx_currency_chunk_trades_%zu.txt", chunk_index++);
                    FILE *fp = fopen(filename, "w");
                    if (!fp) {
                        fprintf(stderr, "[ERROR] Could not open %s for writing\n", filename);
                        continue;
                    }

                    fprintf(fp, "[");
                    size_t written = 0;

                    for (size_t j = i; j < i + chunk_size && j < total; j++) {
                        json_t *item = json_array_get(data, j);
                        const char *instId = json_string_value(json_object_get(item, "instId"));
                        if (instId) {
                            if (written > 0)
                                fprintf(fp, ", ");
                            fprintf(fp, "{\"channel\": \"trades\", \"instId\": \"%s\"}", instId);
                            written++;
                        }
                    }

                    fprintf(fp, "]\n");
                    fclose(fp);
                    // printf("[INFO] Wrote %zu trade instruments to %s\n", written, filename);
                }

                printf("OKX Trade Product IDs saved to okx_currency_chunk_trades_XX.txt\n");
                json_decref(root);
            } else {
                fprintf(stderr, "[ERROR] Invalid or missing 'data' array\n");
            }
        } else {
            fprintf(stderr, "[ERROR] curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        curl_easy_cleanup(curl);
        free(chunk.memory);
    }
}    


void fetch_binance_product_ids_trades_full() {
    CURL *curl;
    CURLcode res;

    struct MemoryStruct chunk = {0};
    chunk.memory = malloc(1);
    chunk.size = 0;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.binance.us/api/v3/exchangeInfo");
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        res = curl_easy_perform(curl);

        if (res == CURLE_OK) {
            json_error_t error;
            json_t *root = json_loads(chunk.memory, 0, &error);        
            json_t *symbols = json_object_get(root, "symbols");

            if (symbols && json_is_array(symbols)) {
                FILE *fp = fopen("currency_text_files/binance_currency_ids_trades.txt", "w");
                if (!fp) {
                    fprintf(stderr, "Failed to open output file\n");
                    json_decref(root);
                    return;
                }

                fprintf(fp, "[");
                for (size_t i = 0; i < json_array_size(symbols); i++) {
                    json_t *entry = json_array_get(symbols, i);
                    const char *symbol = json_string_value(json_object_get(entry, "symbol"));
                    if (symbol) {
                        char lower_symbol[64];
                        strncpy(lower_symbol, symbol, sizeof(lower_symbol) - 1);
                        lower_symbol[sizeof(lower_symbol) - 1] = '\0';
                        for (char *p = lower_symbol; *p; ++p) *p = tolower(*p);

                        fprintf(fp, "\"%s\"", lower_symbol);
                        if (i < json_array_size(symbols) - 1)
                            fprintf(fp, ", ");
                    }
                }
                fprintf(fp, "]\n");
                fclose(fp);

                printf("Binance trade stream symbols saved to binance_currency_ids_trades.txt\n");
                json_decref(root);
            } else {
                fprintf(stderr, "JSON parse error or missing 'symbols' array\n");
            }
        } else {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        curl_easy_cleanup(curl);
        free(chunk.memory);
    }
}

void fetch_binance_product_ids_trades() {
    CURL *curl;
    CURLcode res;

    struct MemoryStruct chunk = {0};
    chunk.memory = malloc(1);
    chunk.size = 0;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.binance.us/api/v3/exchangeInfo");
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        res = curl_easy_perform(curl);

        if (res == CURLE_OK) {
            json_error_t error;
            json_t *root = json_loads(chunk.memory, 0, &error);
            json_t *symbols = json_object_get(root, "symbols");

            if (symbols && json_is_array(symbols)) {
                size_t total = json_array_size(symbols);
                size_t chunk_size = 100;
                size_t chunk_index = 0;

                for (size_t i = 0; i < total; i += chunk_size) {
                    char filename[64];
                    snprintf(filename, sizeof(filename), "currency_text_files/binance_currency_chunk_trades_%zu.txt", chunk_index++);
                    FILE *fp = fopen(filename, "w");
                    if (!fp) {
                        fprintf(stderr, "[ERROR] Could not open %s for writing\n", filename);
                        continue;
                    }

                    size_t written = 0;
                    for (size_t j = i; j < i + chunk_size && j < total; j++) {
                        json_t *entry = json_array_get(symbols, j);
                        const char *symbol = json_string_value(json_object_get(entry, "symbol"));
                        if (symbol) {
                            char lower_symbol[64];
                            strncpy(lower_symbol, symbol, sizeof(lower_symbol) - 1);
                            lower_symbol[sizeof(lower_symbol) - 1] = '\0';
                            for (char *p = lower_symbol; *p; ++p) *p = tolower(*p);
                        
                            fprintf(fp, "%s\n", lower_symbol);
                            written++;
                        }                        
                    }

                    fclose(fp);
                    // printf("[INFO] Wrote %zu symbols to %s\n", written, filename);
                }

                printf("Binance trade stream symbols saved to binance_currency_chunk_trades_XX.txt\n");
                json_decref(root);
            } else {
                fprintf(stderr, "JSON parse error or missing 'symbols' array\n");
            }
        } else {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        curl_easy_cleanup(curl);
        free(chunk.memory);
    }
}

 
 int main() {
     fetch_coinbase_product_ids();
     fetch_huobi_product_ids();
     fetch_okx_product_ids();
     fetch_okx_product_ids_trades();
     fetch_kraken_product_ids();
     fetch_binance_product_ids_trades();

     fetch_huobi_product_ids_full();
     fetch_okx_product_ids_full();
     fetch_okx_product_ids_trades_full();
     fetch_binance_product_ids_trades_full();
     return 0;
 }
 