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
 *  - Writes the formatted list of product IDs (escaped and comma-separated) to `product_ids.txt`.
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
 *  dos2unix fetch_coinbase_id.c
 *  gcc fetch_coinbase_id.c -o fetch_coinbase_id -lcurl -ljansson
 *  ./fetch_coinbase_id
 * 
 * Output:
 *  - `product_ids.txt` will contain a formatted list like:
 *    [\"BTC-USD\", \"ETH-USD\", \"ADA-USD\", ...]
 * 
 * Created: 4/29/2025
 * Updated: 4/29/2025
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
                FILE *fp = fopen("product_ids.txt", "w");
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
                        fprintf(fp, "\\\"%s\\\"", id);
                        if (i < json_array_size(root) - 1) {
                            fprintf(fp, ", ");
                        }
                    }
                }
                fprintf(fp, "]\n");
                fclose(fp);

                printf("Product IDs saved to product_ids.txt\n");
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

int main() {
    fetch_coinbase_product_ids();
    return 0;
}
