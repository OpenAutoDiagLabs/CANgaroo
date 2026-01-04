#!/bin/bash

# CANgaroo Dependency Installer for Linux
# Supports Ubuntu/Debian, Fedora, and Arch Linux

set -e

# Detect Distribution
if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS=$ID
else
    echo "Unsupported Linux distribution."
    exit 1
fi

echo "Detected OS: $OS"

case $OS in
    ubuntu|debian|linuxmint)
        echo "Installing dependencies for $OS (using apt)..."
        sudo apt update
        sudo apt install -y \
            build-essential \
            qt6-base-dev \
            libqt6charts6-dev \
            libqt6serialport6-dev \
            libnl-3-dev \
            libnl-route-3-dev \
            libgl1-mesa-dev \
            pkg-config \
            git
        ;;
    fedora)
        echo "Installing dependencies for Fedora (using dnf)..."
        sudo dnf install -y \
            gcc-c++ \
            make \
            qt6-qtbase-devel \
            qt6-qtcharts-devel \
            qt6-qtserialport-devel \
            libnl3-devel \
            mesa-libGL-devel \
            pkgconf-pkg-config \
            git
        ;;
    arch)
        echo "Installing dependencies for Arch Linux (using pacman)..."
        sudo pacman -S --needed --noconfirm \
            base-devel \
            qt6-base \
            qt6-charts \
            qt6-serialport \
            libnl \
            mesa \
            pkgconf \
            git
        ;;
    *)
        echo "Error: Distribution $OS is not explicitly supported by this script."
        echo "Please install Qt6 (base, charts, serialport) and libnl3 manually."
        exit 1
        ;;
esac

echo "-------------------------------------------------------"
echo "Dependencies installed successfully!"
echo "To build CANgaroo, run:"
echo "  cd src"
echo "  PKG_CONFIG_PATH=/usr/lib/x86_64-linux-gnu/pkgconfig qmake6"
echo "  make -j\$(nproc)"
echo "-------------------------------------------------------"
