#!/bin/bash
# ESP32-CAM Flashing Script
# Prerequisites: PlatformIO CLI installed

# Check if port is provided
if [ -z "$1" ]; then
  echo "Usage: ./flash.sh <PORT>"
  echo "Example: ./flash.sh /dev/ttyUSB0"
  exit 1
fi

PORT=$1

echo "Flashing ESP32-CAM firmware to $PORT..."
echo "Note: Make sure GPIO0 is connected to GND before powering up to enter flash mode"
echo "Press any key when the ESP32-CAM is ready for flashing..."
read -n 1

# Build and upload using PlatformIO
pio run -t upload --upload-port $PORT

# Check if the upload was successful
if [ $? -eq 0 ]; then
  echo "Firmware successfully flashed!"
  echo "Disconnect GPIO0 from GND and reset the ESP32-CAM to run the firmware"
else
  echo "Error flashing firmware. Please check connections and try again."
fi
