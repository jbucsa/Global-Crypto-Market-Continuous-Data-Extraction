/*
 * Coinbase Product ID Fetcher
 * 
 * This utility fetches all available trading pairs (product IDs) from the 
 * Coinbase Exchange REST API and saves them to a text file in a format suitable 
 * for WebSocket subscription payloads.
 * 
 * Features:
 *  - Connects to the Coinbase public products API endpoint.
 *  - Parses the JSON response to extract all product IDs.
 *  - Writes the formatted list of product IDs (escaped and comma-separated) to `currency_ids.txt`.
 * 
 * Dependencies:
 *  - libcurl: For performing HTTPS requests.
 *  - jansson: For parsing JSON responses.
 *  - Standard C libraries (stdio, stdlib, string).
 * 
 * Usage:
 *  - Run once to generate a list of all current Coinbase trading pairs.
 *  - Output can be copied into a WebSocket subscription request.
 * 
 * Build & Run:
 *  sudo apt install libcurl4-openssl-dev libjansson-dev build-essential
 *  dos2unix fetch_currency_id.c
 *  gcc fetch_currency_id.c -o fetch_currency_id -lcurl -ljansson
 *  ./fetch_currency_id
 * 
 * Output:
 *  - `currency_ids.txt` will contain a formatted list like:
 *    [\"BTC-USD\", \"ETH-USD\", \"ADA-USD\", ...]
 * 
 * Created: 4/29/2025
 * Updated: 5/4/2025
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
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



 
 int main() {
     fetch_coinbase_product_ids();
     fetch_huobi_product_ids();
     fetch_okx_product_ids();
     fetch_kraken_product_ids();

     fetch_huobi_product_ids_full();
     return 0;
 }
 