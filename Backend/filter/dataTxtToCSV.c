// Note the current issues with this file are the following:
// 1. The Input file for the text data needs to be in the filter directory and names "output_data.txt"
// 2. The code counter method us,ed to identify if the "unknown" values from Kraken's Exchange are "ADA-USD", "BTC-USD", or "ETH-USD" does not run properly.

// To run this code, type the following in the base lines
// gcc your_program.c -o your_program
// ./your_program /path/to/input.txt /path/to/output.csv


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct {
    char *timestamp;
    char *exchange;
    char *product;
    double price;
} Entry;

typedef struct {
    char *key;
    char *value;
} ProductMapping;

typedef struct {
    char *product;
    double value;
    int initialized;
} PriceCounter;

ProductMapping product_mappings[] = {
    {"tBTCUSD", "BTC-USD"},
    {"BTCUSDT", "BTC-USD"},
    {"ADAUSDT", "ADA-USD"},
    {"ETHUSDT", "ETH-USD"},
    {NULL, NULL}
};

PriceCounter price_counters[] = {
    {"ADA-USD", 0.0, 0},
    {"BTC-USD", 0.0, 0},
    {"ETH-USD", 0.0, 0}
};

int compare_entries(const void *a, const void *b) {
    const Entry *entry_a = (const Entry *)a;
    const Entry *entry_b = (const Entry *)b;
    return strcmp(entry_a->timestamp, entry_b->timestamp);
}

void free_entry(Entry *entry) {
    free(entry->timestamp);
    free(entry->exchange);
    free(entry->product);
}

Entry *parse_line(char *line) {
    char *timestamp_start = strchr(line, '[');
    if (!timestamp_start) return NULL;
    timestamp_start += 1;
    char *timestamp_end = strchr(timestamp_start, ']');
    if (!timestamp_end) return NULL;
    size_t timestamp_len = timestamp_end - timestamp_start;
    char *timestamp = malloc(timestamp_len + 1);
    strncpy(timestamp, timestamp_start, timestamp_len);
    timestamp[timestamp_len] = '\0';

    char *exchange_start = timestamp_end + 2;
    if (*exchange_start != '[') {
        free(timestamp);
        return NULL;
    }
    exchange_start += 1;
    char *exchange_end = strchr(exchange_start, ']');
    if (!exchange_end) {
        free(timestamp);
        return NULL;
    }
    size_t exchange_len = exchange_end - exchange_start;
    char *exchange = malloc(exchange_len + 1);
    strncpy(exchange, exchange_start, exchange_len);
    exchange[exchange_len] = '\0';

    char *product_start = exchange_end + 2;
    if (*product_start != '[') {
        free(timestamp);
        free(exchange);
        return NULL;
    }
    product_start += 1;
    char *product_end = strchr(product_start, ']');
    if (!product_end) {
        free(timestamp);
        free(exchange);
        return NULL;
    }
    size_t product_len = product_end - product_start;
    char *product = malloc(product_len + 1);
    strncpy(product, product_start, product_len);
    product[product_len] = '\0';

    char *price_start = strstr(product_end, "Price: ");
    if (!price_start) {
        free(timestamp);
        free(exchange);
        free(product);
        return NULL;
    }
    price_start += 7;
    double price = strtod(price_start, NULL);

    Entry *entry = malloc(sizeof(Entry));
    entry->timestamp = timestamp;
    entry->exchange = exchange;
    entry->product = product;
    entry->price = price;

    return entry;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input_file> <output_file>\n", argv[0]);
        return 1;
    }

    FILE *input = fopen(argv[1], "r");
    if (!input) {
        perror("Error opening input file");
        return 1;
    }

    Entry *entries = NULL;
    size_t num_entries = 0;
    size_t capacity = 0;

    char *line = NULL;
    size_t line_len = 0;
    while (getline(&line, &line_len, input) != -1) {
        char *trimmed_line = line;
        while (*trimmed_line == ' ' || *trimmed_line == '\t') trimmed_line++;
        size_t len = strlen(trimmed_line);
        while (len > 0 && (trimmed_line[len-1] == '\n' || trimmed_line[len-1] == '\r'))
            trimmed_line[--len] = '\0';
        if (len == 0) continue;

        Entry *entry = parse_line(trimmed_line);
        if (!entry) {
            fprintf(stderr, "Skipping invalid line: %s\n", trimmed_line);
            continue;
        }

        if (num_entries >= capacity) {
            capacity = capacity ? capacity * 2 : 16;
            entries = realloc(entries, capacity * sizeof(Entry));
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

    qsort(entries, num_entries, sizeof(Entry), compare_entries);

    for (size_t i = 0; i < num_entries; i++) {
        Entry *entry = &entries[i];
        for (ProductMapping *m = product_mappings; m->key; m++) {
            if (strcmp(entry->product, m->key) == 0) {
                free(entry->product);
                entry->product = strdup(m->value);
                break;
            }
        }

        for (int j = 0; j < 3; j++) {
            if (strcmp(entry->product, price_counters[j].product) == 0) {
                price_counters[j].value = entry->price;
                price_counters[j].initialized = 1;
                break;
            }
        }

        if (strcmp(entry->product, "unknown") == 0) {
            double min_diff = INFINITY;
            int closest = -1;
            for (int j = 0; j < 3; j++) {
                if (price_counters[j].initialized) {
                    double diff = fabs(entry->price - price_counters[j].value);
                    if (diff < min_diff) {
                        min_diff = diff;
                        closest = j;
                    }
                }
            }
            if (closest != -1) {
                free(entry->product);
                entry->product = strdup(price_counters[closest].product);
            }
        }
    }

    FILE *output = fopen(argv[2], "w");
    if (!output) {
        perror("Error opening output file");
        return 1;
    }

    fprintf(output, "index,time,exchange,product,price\n");
    for (size_t i = 0; i < num_entries; i++) {
        Entry *entry = &entries[i];
        fprintf(output, "%zu,%s,%s,%s,%.8f\n", i + 1, entry->timestamp, entry->exchange, entry->product, entry->price);
    }

    fclose(output);

    for (size_t i = 0; i < num_entries; i++) {
        free_entry(&entries[i]);
    }
    free(entries);

    return 0;
}