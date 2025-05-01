# WebSocket Market Data

Currently Supported Exchanges:
- Binance (Ticker + Trade)
- Coinbase (Ticker + Trade)
- Kraken (Ticker)
- Bitfinex (Ticker)
- Huobi (Ticker)
- OKX (Ticker)

---

## Dependencies

This project requires the following libraries and tools to be installed:

### System Packages (Linux / WSL)
```sh
sudo apt update && sudo apt install -y \
    libjansson-dev \
    libwebsockets-dev \
    build-essential \
    cmake \
    pkg-config
```

### Windows (via WSL or MinGW)
1. Use [WSL](https://learn.microsoft.com/en-us/windows/wsl/) or [MinGW](https://www.mingw-w64.org/).
2. Install libraries using `vcpkg`:
```sh
vcpkg install jansson libwebsockets
```

---

## Installing and Setting Up libwebsockets

The project relies heavily on the `libwebsockets` library for managing WebSocket connections. If the system package version is outdated or unavailable, you can build it from source:

### Build from Source
```sh
git clone https://libwebsockets.org/repo/libwebsockets
cd libwebsockets
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DLWS_WITH_STATIC=OFF
make
sudo make install
```

### Additional Setup Instructions for WSL Users

1. Open your terminal or VS Code terminal and install WSL if needed:
```sh
wsl --install
```

2. Ensure required build tools and libraries are installed:
```sh
sudo apt update
sudo apt install libwebsockets-dev
sudo apt install build-essential
sudo apt install cmake
```

3. Navigate to your working directory in WSL (e.g., `/mnt/c/Users/your-name/your-project`)

4. Clone the `libwebsockets` repo into the desired folder:
```sh
git clone https://libwebsockets.org/repo/libwebsockets
```

5. Build and run a minimal example to test:
```sh
cd libwebsockets
mkdir build
cd build
cmake ..
make
cd bin
./lws-minimal-ss-server-hello_world
```

### Notes
- `libwebsockets` must be compiled with SSL support (enabled by default).
- Ensure `pkg-config` can locate the installed `.pc` file:
```sh
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH
```
- To verify installation:
```sh
pkg-config --modversion libwebsockets
```

# MongoDB Database Tools Manual Installation (Ubuntu 24.04 / Noble)

Since MongoDB has not released official packages for Ubuntu Noble yet, follow these steps to install `bsondump`, `mongodump`, and other tools manually:

---

## 1. Download the MongoDB Database Tools

```bash
wget https://fastdl.mongodb.org/tools/db/mongodb-database-tools-ubuntu2004-x86_64-100.9.4.tgz
```

> Note: The package is labeled for Ubuntu 20.04, but it works on 24.04.

---

## 2. Extract the archive

```bash
tar -zxvf mongodb-database-tools-ubuntu2004-x86_64-100.9.4.tgz
```

---

## 3. Install the binaries

Move the extracted tools to your system path:

```bash
sudo cp mongodb-database-tools-ubuntu2004-x86_64-100.9.4/bin/* /usr/local/bin/
```

---

## 4. Verify the installation

Check if `bsondump` is available:

```bash
bsondump --help
```

You should now have access to:
- `bsondump`
- `mongodump`
- `mongorestore`
- `mongoimport`
- `mongoexport`
- (and others)

---

# MongoDB Summary

This lets you use MongoDB database utilities without needing full MongoDB server installation, and works on newer systems like Ubuntu 24.04 where apt packages are not yet published.


Then run:
```bash
rm -rf mongodb-database-tools-ubuntu2004-x86_64-100.9.4/
rm mongodb-database-tools-ubuntu2004-x86_64-100.9.4.tgz
```

To remove the file in the current folder.


For bson run:
```bash
sudo apt install libbson-dev
```

### Troubleshooting: Common WSL Errors

**Error:** `Failed to take /etc/passwd lock: Invalid argument`

This can happen if installation of required packages was interrupted or incomplete. To fix:
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

To clean up old builds:
```sh
make clean
```

This will compile the following components:
- `main.c`
- `exchange_websocket.c`
- `exchange_connect.c`
- `exchange_reconnect.c`
- `json_parser.c`
- `utils.c`

Resulting in a `crypto_ws` executable.

---

## Running the WebSocket Data Processor

To start streaming and logging market data:
```sh
./crypto_ws
```

This will:
- Connect to all supported exchanges
- Log **ticker updates** to `ticker_output_data.json`
- Log **trade updates** to `trades_output_data.json`

You can stop it anytime with `Ctrl+C`.

---

## Logs and Debugging

- Errors and critical logs are printed to the terminal.
- Runtime logs are saved in `error_log.txt` (if implemented).
- JSON log files are appended continuously.

