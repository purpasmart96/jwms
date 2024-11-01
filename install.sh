#!/bin/sh

BIN1="jwm-helper"
BIN2="jwms"
DESKTOP="jwms.desktop"
CONF="jwms.conf"

DESKTOP_DIR="/usr/share/xsessions"
BIN_DIR="/usr/local/bin"
SYS_CONF_DIR="/etc/jwms"

# Ensure the user has root privileges
if [ "$(id -u)" -ne 0 ]; then
    echo "This script requires root privileges. Please run with sudo/doas."
    exit 1
fi

echo "Installing $BIN1 to $BIN_DIR..."
install -D -m 755 $BIN1 $BIN_DIR/$BIN1

echo "Installing $BIN2 to $BIN_DIR..."
install -D -m 755 $BIN2 $BIN_DIR/$BIN2

echo "Installing $DESKTOP to $DESKTOP_DIR..."
install -m 644 $DESKTOP $DESKTOP_DIR/$DESKTOP

echo "Installing $CONF to $SYS_CONF_DIR..."
install -D -m 644 $CONF $SYS_CONF_DIR/$CONF

echo "Installation complete!"
