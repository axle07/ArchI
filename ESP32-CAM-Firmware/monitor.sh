#!/bin/bash
# ESP32-CAM Serial Monitor Script

# Check if port is provided
if [ -z "$1" ]; then
  PORT=$(ls -1 /dev/cu* | grep usbserial)
  if [ -z "$PORT" ]; then
    echo "No USB serial device found."
    exit 1
  fi
else
  PORT=$1
fi

echo "Connecting to ESP32-CAM on $PORT..."
echo "Press Ctrl+A, then Ctrl+\ to exit screen"
echo "----------------------------------------"
sleep 1

# Connect to the serial port
screen $PORT 115200
