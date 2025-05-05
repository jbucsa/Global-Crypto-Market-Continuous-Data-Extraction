Here is the updated `.md` file with all necessary adjustments and explanations:

````markdown
# WebSocket Market Data

A multi-exchange cryptocurrency market data aggregator using WebSocket APIs.

## Currently Supported Exchanges:
- Binance (Ticker)
- Coinbase (Ticker)
- Kraken (Ticker)
- Huobi (Ticker)
- OKX (Ticker)
- Bitfinex (Planned)

---

## Dependencies

This project requires several development libraries and tools:

## Quick Setup: Installing Dependencies

To install all necessary libraries and tools in one step, use the included setup script:

```sh
./setup_dependencies.sh
```

This script will:

* Update your system package index.
* Install all required dev libraries (`libwebsockets`, `jansson`, `curl`, `zlib`, etc.).
* Ensure build tools like `gcc`, `make`, and `pkg-config` are installed.
* Output a success message once ready.

**Note:**
If you are on Ubuntu 24.04 and `libbson-1.0-dev` fails to install, the script falls back to `libbson-dev`.

### System Packages (Linux / WSL)

For manual setup install the following packages using `apt`:
```sh
sudo apt update && sudo apt install -y \
    build-essential \
    libjansson-dev \
    libwebsockets-dev \
    libcurl4-openssl-dev \
    libbson-dev \
    zlib1g-dev \
    dos2unix \
    pkg-config \
    git
````

> `libbson-dev` is the correct package for BSON handling. `libbson-1.0-dev` may not exist.

### Windows (via WSL or MinGW)

1. Use [WSL](https://learn.microsoft.com/en-us/windows/wsl/) or [MinGW](https://www.mingw-w64.org/).
2. Install libraries via [vcpkg](https://github.com/microsoft/vcpkg):

```sh
vcpkg install jansson libwebsockets
```

---

## Installing and Setting Up `libwebsockets` from Source

If your system’s version is outdated or missing, build manually:

```sh
git clone https://libwebsockets.org/repo/libwebsockets
cd libwebsockets
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DLWS_WITH_STATIC=OFF
make
sudo make install
```

---

### WSL Setup Instructions

```sh
wsl --install
sudo apt update
sudo apt install libwebsockets-dev build-essential cmake
```

Clone and build libwebsockets:

```sh
git clone https://libwebsockets.org/repo/libwebsockets
cd libwebsockets
mkdir build && cd build
cmake ..
make
```

To run a minimal example:

```sh
cd bin
./lws-minimal-ss-server-hello_world
```

### Notes

* `libwebsockets` must support SSL (enabled by default).
* Make sure `pkg-config` can locate `.pc` files:

```sh
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH
```

* Verify installation:

```sh
pkg-config --modversion libwebsockets
```

---

# MongoDB Tools for Ubuntu 24.04 (Manual Setup)

MongoDB has no official `.deb` packages for Ubuntu Noble (24.04). Use the tools directly:

### 1. Download Tools

```sh
wget https://fastdl.mongodb.org/tools/db/mongodb-database-tools-ubuntu2004-x86_64-100.9.4.tgz
```

### 2. Extract

```sh
tar -zxvf mongodb-database-tools-ubuntu2004-x86_64-100.9.4.tgz
```

### 3. Install Binaries

```sh
sudo cp mongodb-database-tools-ubuntu2004-x86_64-100.9.4/bin/* /usr/local/bin/
```

### 4. Verify

```sh
bsondump --help
```

### 5. Clean Up

```sh
rm -rf mongodb-database-tools-ubuntu2004-x86_64-100.9.4/
rm mongodb-database-tools-ubuntu2004-x86_64-100.9.4.tgz
```

---

## MongoDB Summary

These tools let you use BSON utilities like `bsondump` and `mongodump` without needing a full MongoDB install. Useful on newer distros lacking official packages.

To install BSON C library:

```sh
sudo apt install libbson-dev
```

---

## Troubleshooting (WSL)

**Issue:** `Failed to take /etc/passwd lock: Invalid argument`

Fix:

```sh
sudo mv /var/lib/dpkg/info /var/lib/dpkg/info_silent
sudo mkdir /var/lib/dpkg/info
sudo apt-get update
sudo apt-get -f install
sudo mv /var/lib/dpkg/info/* /var/lib/dpkg/info_silent
sudo rm -rf /var/lib/dpkg/info
sudo mv /var/lib/dpkg/info_silent /var/lib/dpkg/info
sudo apt-get update
```

---

## Compilation & Setup

To build the project:

```sh
make
```

To clean:

```sh
make clean
```

This compiles:

* `main.c`
* `exchange_websocket.c`
* `exchange_connect.c`
* `exchange_reconnect.c`
* `json_parser.c`
* `utils.c`

Output:

* `crypto_ws` (main WebSocket executable)
* `fetch_currency_id` (runs symbol fetcher at build)

---

## Running the Market Data Logger

Start the program:

```sh
./crypto_ws
```

What it does:

* Subscribes to all supported exchanges
* Logs tickers to `ticker_output_data.json`
* Logs trades to `trades_output_data.json`

Stop with `Ctrl+C`.

---

## Logs & Output

* Terminal output includes connection and error messages.
* JSON logs are continuously written and flushed to disk.
* BSON files are created in `bson_output/` by date per exchange.