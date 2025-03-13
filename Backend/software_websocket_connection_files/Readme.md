# WebSocket Market Data
Supported Exchanges:
- Binance (Ticker + Trade)
- Coinbase (Ticker + Trade)
- Kraken (Ticker)
- Bitfinex (Ticker)
- Huobi (Ticker)
- OKX (Ticker)

## Dependencies
This project requires the following libraries:

### System Packages
```sh
sudo apt update && sudo apt install -y \
    libjansson-dev \
    libwebsockets-dev \
    build-essential \
    cmake \
    pkg-config
```

### Windows (WSL or MinGW)
1. Install [MinGW] or use [WSL].
2. Install dependencies manually or use `vcpkg`:
```sh
vcpkg install jansson libwebsockets
```

## Compilation & Setup
To build the project, run:
```sh
make
```

To clean up old builds:
```sh
make clean
```

## Running the WebSocket Data Processor
Start the WebSocket connection to fetch market data:
```sh
./crypto_ws
```

This will:
- Connect to all exchanges specified.
- Log ticker updates to `ticker_output_data.json`.
- Log trade updates to `trade_output_data.json`.

## Logs and Debugging
All error messages and logs are saved in `error_log.txt`.
