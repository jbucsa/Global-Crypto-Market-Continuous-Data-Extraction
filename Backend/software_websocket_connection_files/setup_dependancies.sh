#!/bin/bash

# This script installs all required system dependencies for building and running
# the crypto_ws WebSocket-based cryptocurrency data logger on Debian/Ubuntu systems.

echo "[INFO] Installing system dependencies for crypto_ws..."

# Update package list
sudo apt update

# Install core build tools and libraries
sudo apt install -y \
    build-essential \
    libjansson-dev \
    libcurl4-openssl-dev \
    libwebsockets-dev \
    libbson-dev \
    zlib1g-dev \
    dos2unix \
    pkg-config \
    git

echo "[SUCCESS] All required packages installed. You can now run: make"
