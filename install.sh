#!/usr/bin/env bash
set -euo pipefail

REPO="deepajai/clive"
VERSION="${1:-latest}"
INSTALL_DIR="${2:-/usr/local/bin}"

if [ "$VERSION" = "latest" ]; then
    DOWNLOAD_URL="https://github.com/$REPO/releases/latest/download"
else
    DOWNLOAD_URL="https://github.com/$REPO/releases/download/$VERSION"
fi

# Detect OS and architecture
OS="$(uname -s | tr '[:upper:]' '[:lower:]')"
ARCH="$(uname -m)"

case "$OS" in
    linux)   OS="linux" ;;
    darwin)  OS="macos" ;;
    *)
        echo "Unsupported OS: $OS"
        exit 1
        ;;
esac

case "$ARCH" in
    x86_64|amd64) ARCH="x86_64" ;;
    aarch64|arm64) ARCH="arm64" ;;
    *)
        echo "Unsupported architecture: $ARCH"
        exit 1
        ;;
esac

BINARY_NAME="clive-$OS-$ARCH"
URL="$DOWNLOAD_URL/$BINARY_NAME"

echo "  Clive $VERSION ($OS-$ARCH)"
echo "  Downloading from $URL ..."

# Download the binary
if command -v curl &>/dev/null; then
    curl -fsSL "$URL" -o "$BINARY_NAME"
elif command -v wget &>/dev/null; then
    wget -q "$URL" -O "$BINARY_NAME"
else
    echo "Error: need curl or wget to download"
    exit 1
fi

chmod +x "$BINARY_NAME"

# Install
if [ -w "$INSTALL_DIR" ]; then
    mv "$BINARY_NAME" "$INSTALL_DIR/clive"
    echo "  Installed to $INSTALL_DIR/clive"
else
    echo "  Installing with sudo to $INSTALL_DIR ..."
    sudo mv "$BINARY_NAME" "$INSTALL_DIR/clive"
    echo "  Installed to $INSTALL_DIR/clive"
fi

echo ""
echo "  Clive is ready! Run: clive filename.txt"
