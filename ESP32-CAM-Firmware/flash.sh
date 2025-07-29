#!/bin/bash
# ESP32-CAM Flashing Script
# Prerequisites: PlatformIO CLI installed

# Check if port is provided
if [ -z "$1" ]; then
  PORT=$(ls -d1 /dev/* | grep cu\.usbserial)
else
  PORT=$1
fi

if [ -z "$PORT" ]; then
  echo "No USB serial device found."
  exit 1
fi

echo "Flashing ESP32-CAM firmware to $PORT..."
echo "Note: Make sure GPIO0 is connected to GND before powering up to enter flash mode"


# Build and upload using PlatformIO - only flash the main environment (not test)
pio run -e esp32cam -t upload --upload-port $PORT

# Check if the upload was successful
if [ $? -eq 0 ]; then
  echo "Firmware successfully flashed!"
  echo "Disconnect GPIO0 from GND and reset the ESP32-CAM to run the firmware"
else
  echo "Error flashing firmware. Please check connections and try again."
fi
