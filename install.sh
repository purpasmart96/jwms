#!/bin/sh

BIN="jwms"
CONF="jwms.conf"
DEST_DIR="/usr/local/bin"
SYS_CONF_DIR="/etc/jwms"

# Ensure the user has root privileges
if [ "$(id -u)" -ne 0 ]; then
    echo "This script requires root privileges. Please run with sudo/doas."
    exit 1
fi

echo "Installing $BIN to $DEST_DIR..."
install -D -m 755 $BIN $DEST_DIR/$BIN

echo "Installing $CONF to $SYS_CONF_DIR..."
install -D -m 644 $CONF $SYS_CONF_DIR/$CONF

echo "Installation complete!"
