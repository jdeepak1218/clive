#!/usr/bin/env bash
set -euo pipefail

REPO="jdeepak1218/clive"
INSTALL_DIR="${2:-/usr/local/bin}"

show_version() {
    VERSION="${1:-latest}"
    if [ "$VERSION" = "latest" ]; then
        echo "Clive latest (from $REPO)"
    else
        echo "Clive $VERSION (from $REPO)"
    fi
}

do_install() {
    local VERSION="${1:-latest}"

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
}

# Entry point
case "${1:-}" in
    --update|-u|update)
        shift
        echo "Updating Clive..."
        do_install "${1:-latest}"
        ;;
    --version|-v|version)
        show_version "${2:-}"
        ;;
    --help|-h|help)
        echo "Clive Installer"
        echo "Usage: curl -fsSL https://raw.githubusercontent.com/$REPO/main/install.sh | bash [--] [version] [install_dir]"
        echo ""
        echo "Commands:"
        echo "  (no args)         Install the latest version"
        echo "  <version>         Install a specific version (e.g., v1.0.0)"
        echo "  update            Update to the latest version"
        echo "  version           Show version info"
        echo ""
        echo "Examples:"
        echo "  curl -fsSL https://raw.githubusercontent.com/$REPO/main/install.sh | bash"
        echo "  curl -fsSL https://raw.githubusercontent.com/$REPO/main/install.sh | bash -s v1.0.0"
        echo "  curl -fsSL https://raw.githubusercontent.com/$REPO/main/install.sh | bash -s update"
        ;;
    *)
        do_install "${1:-latest}"
        ;;
esac
