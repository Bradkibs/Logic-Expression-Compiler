#!/bin/bash
# install.sh - Install dependencies for Logical Expressions Compiler
# Author: Cascade AI

set -e  # Exit immediately if a command exits with non-zero status

echo "=== Installing dependencies for Logical Expressions Compiler ==="

# Detect OS
if [ -f /etc/os-release ]; then
    # Linux
    . /etc/os-release
    OS=$NAME
elif [ "$(uname)" == "Darwin" ]; then
    # macOS
    OS="macOS"
else
    echo "Unsupported operating system"
    exit 1
fi

install_ubuntu_deps() {
    echo "Installing dependencies for Ubuntu/Debian..."
    sudo apt-get update
    sudo apt-get install -y \
        build-essential \
        flex \
        bison \
        gcc \
        clang \
        llvm
    echo "Dependencies installed successfully for Ubuntu/Debian."
}

install_fedora_deps() {
    echo "Installing dependencies for Fedora/RHEL/CentOS..."
    sudo dnf update
    sudo dnf install -y \
        make \
        flex \
        bison \
        gcc \
        clang \
        llvm
    echo "Dependencies installed successfully for Fedora/RHEL/CentOS."
}

install_macos_deps() {
    echo "Installing dependencies for macOS..."
    # Check if Homebrew is installed, install if not
    if ! command -v brew &> /dev/null; then
        echo "Homebrew not found. Installing Homebrew..."
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    fi
    
    brew update
    brew install \
        flex \
        bison \
        llvm
    echo "Dependencies installed successfully for macOS."
}

# Install dependencies based on OS
case "$OS" in
    *"Ubuntu"*|*"Debian"*)
        install_ubuntu_deps
        ;;
    *"Fedora"*|*"CentOS"*|*"Red Hat"*)
        install_fedora_deps
        ;;
    "macOS")
        install_macos_deps
        ;;
    *)
        echo "Unsupported distribution: $OS"
        echo "Please install the following packages manually:"
        echo "  - flex"
        echo "  - bison"
        echo "  - gcc"
        echo "  - clang"
        echo "  - llvm"
        exit 1
        ;;
esac

echo "=== All dependencies installed successfully ==="
echo "You can now build the Logical Expressions Compiler with 'make'"
