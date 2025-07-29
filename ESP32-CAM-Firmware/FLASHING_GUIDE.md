# ESP32-CAM Flashing Guide

## Hardware Connection

1. **Wiring for Programming Mode:**
   ```
   ESP32-CAM         | USB-to-TTL Converter
   -----------------|-------------------------
   5V (or 3V3)      | 5V (or 3.3V)
   GND              | GND
   U0R (RX)         | TX
   U0T (TX)         | RX
   GPIO0            | GND (only during flashing)
   ```

2. **Flashing Procedure:**
   - Connect GPIO0 to GND
   - Press the RST button on the ESP32-CAM (or power cycle it)
   - Run the flash script
   - After flashing is complete, disconnect GPIO0 from GND
   - Press the RST button again (or power cycle)

## Troubleshooting

### Common Issues:

1. **"Failed to connect to ESP32" Error:**
   - Make sure GPIO0 is connected to GND when powering up the device
   - Try pressing the RST button right before running the flash script
   - Check your wiring, especially RX/TX (they should be crossed: RX→TX, TX→RX)
   - Try a different USB port
   - Try a slower upload speed (edit platformio.ini: upload_speed = 115200)

2. **LED Behavior:**
   - If the LED is constantly ON: The ESP32-CAM is likely in bootloader mode
   - If the LED is blinking: The firmware is running

3. **Serial Monitor Connection:**
   - After flashing, you can monitor the device's output using:
   ```
   screen /dev/cu.usbserial-A5XK3RJT 115200
   ```
   - To exit screen: Press Ctrl+A, then Ctrl+\, then y

4. **Power Issues:**
   - ESP32-CAM requires sufficient power (at least 500mA)
   - USB-to-TTL converters might not provide enough power
   - Consider using an external 5V power supply for stable operation

## Manual Flashing Commands

If the flash script doesn't work, try these manual commands:

```bash
# Put ESP32-CAM in bootloader mode (GPIO0→GND, then press RST)
cd /Users/alexjohnson/dev/ArchI/ESP32-CAM-Firmware
pio run -e esp32cam -t upload --upload-port /dev/cu.usbserial-A5XK3RJT
```

## Serial Monitor

To monitor the ESP32-CAM's output after flashing:

```bash
# Disconnect GPIO0 from GND, press RST, then run:
screen /dev/cu.usbserial-A5XK3RJT 115200
```
