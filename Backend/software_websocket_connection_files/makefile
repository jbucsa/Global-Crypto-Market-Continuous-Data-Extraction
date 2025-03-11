# Makefile for Crypto Exchange WebSocket Client
#
# This Makefile automates the compilation and linking of the WebSocket-based
# cryptocurrency market data logger. It compiles multiple source files and 
# links necessary libraries to produce the `crypto_ws` executable.
#
# Components:
#  - `main.c`: Initializes the WebSocket connections and handles application logic.
#  - `exchange_websocket.c`: Manages WebSocket connections and message handling.
#  - `json_parser.c`: Provides JSON data extraction functions.
#  - `utils.c`: Includes utility functions for timestamp conversion, logging, and CSV processing.
#
# Compilation:
#  - Uses `gcc` with `-Wall -Wextra` for additional warnings.
#  - Includes the Jansson and libwebsockets libraries (`-ljansson -lwebsockets -lm`).
#
# Targets:
#  - `all`: Compiles all source files and creates the `crypto_ws` executable.
#  - `clean`: Removes compiled object files and the executable.
#
# Usage:
#  - To build the program: `make`
#  - To clean compiled files: `make clean`
#
# Created: 2/26/2025
# Updated: 3/7/2025

CC = gcc
CFLAGS = -Wall -Wextra -I.
LIBS = -ljansson -lwebsockets -lm

all: crypto_ws

crypto_ws: main.o exchange_websocket.o json_parser.o utils.o
	$(CC) -o crypto_ws main.o exchange_websocket.o json_parser.o utils.o $(LIBS)

main.o: main.c exchange_websocket.h utils.h
	$(CC) $(CFLAGS) -c main.c

exchange_websocket.o: exchange_websocket.c exchange_websocket.h json_parser.h utils.h
	$(CC) $(CFLAGS) -c exchange_websocket.c

json_parser.o: json_parser.c json_parser.h
	$(CC) $(CFLAGS) -c json_parser.c

utils.o: utils.c utils.h
	$(CC) $(CFLAGS) -c utils.c

clean:
	rm -f *.o crypto_ws
