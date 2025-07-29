# ArchI ESP32-CAM Streaming System

A real-time, low-latency camera streaming system using ESP32-CAM, .NET Core backend, and React frontend.

## Project Overview

This project creates an ultra-low latency video streaming system with the following components:

1. **ESP32-CAM**: Captures and streams camera footage over WiFi
2. **C# .NET Core Backend**: Processes the video stream and serves it to clients
3. **React Frontend**: Displays the camera feed in a web browser

## Architecture

For a detailed architecture overview, see the [Architecture Diagram](docs/architecture_diagram.md) documentation.

### Data Flow

1. The ESP32-CAM captures frames from its camera sensor
2. Frames are compressed as JPEG and sent via HTTP POST with authentication token to the server
3. The .NET Core backend validates the token, receives the frames and broadcasts them to connected clients via SignalR
4. The React frontend displays the frames in real-time using a WebSocket connection

### System Components

- **ESP32-CAM**: Edge device with camera module, running custom firmware
- **Backend Server**: .NET Core application running on port 5001, supporting authentication
- **Frontend**: React application with SignalR client for real-time updates

## Setup Instructions

### Prerequisites

- ESP32-CAM board (AI-Thinker model with OV2640 camera)
- USB-to-TTL converter for programming the ESP32-CAM
- macOS or Linux machine for hosting the .NET Core backend (currently configured for macOS)
- Node.js for running the React frontend
- Micro SD card (16GB+ recommended)
- Power supplies for ESP32-CAM and server hardware
- Development computer with:
  - Visual Studio Code with PlatformIO extension
  - .NET SDK 7.0 or higher
  - Node.js and npm

### ESP32-CAM Setup

1. **Hardware Connections**:
   
   For programming:
   ```
   ESP32-CAM    | USB-to-TTL Converter
   -------------|-----------------------
   5V           | 5V (or 3.3V if 5V not available)
   GND          | GND
   U0R (RX)     | TX
   U0T (TX)     | RX
   GPIO0        | GND (only during programming)
   ```
   
   After programming, disconnect GPIO0 from GND to run the program.

2. **Software Setup**:

   a. Install Visual Studio Code and the PlatformIO extension
   b. Open the ESP32-CAM-Firmware project folder in VS Code
   c. Edit the WiFi and server settings in `src/main.cpp`:
      ```cpp
      const char* ssid = "YOUR_WIFI_SSID";
      const char* password = "YOUR_WIFI_PASSWORD";
      const char* serverAddress = "YOUR_RASPBERRY_PI_IP";
      ```
   d. Connect the ESP32-CAM to your computer with the USB-to-TTL converter
   e. Hold the RESET button on the ESP32-CAM, then press the UPLOAD button in PlatformIO
   f. After uploading completes, disconnect GPIO0 from GND and reset the board

### Server Setup

1. **Operating System**:
   
   a. Install macOS or a Linux distribution (64-bit recommended)
   b. Configure SSH and network settings

2. **Security Hardening**:

   a. Update the system:
      ```bash
      sudo apt update && sudo apt upgrade -y
      ```
      
   b. Change the default password:
      ```bash
      passwd
      ```
      
   c. Configure the firewall:
      ```bash
      sudo apt install -y ufw
      sudo ufw allow ssh
      sudo ufw allow 5000/tcp  # For the web server
      sudo ufw enable
      ```
      
   d. Create a dedicated user for the application:
      ```bash
      sudo adduser archi
      sudo usermod -aG sudo archi
      ```

3. **Backend Setup**:

   a. Install .NET 7.0 SDK:
      ```bash
      curl -sSL https://dot.net/v1/dotnet-install.sh | bash /dev/stdin --channel 7.0
      echo 'export PATH=$PATH:$HOME/.dotnet' >> ~/.bashrc
      source ~/.bashrc
      ```
      
   b. Copy the WebServer/Backend folder to the server
   
   c. Build and run the application:
      ```bash
      cd /path/to/WebServer/Backend
      dotnet build
      dotnet run --urls=http://0.0.0.0:5001
      ```
      
    d. To run as a service on startup:

        1. Create a systemd service file:
                ```bash
                sudo tee /etc/systemd/system/archi-server.service > /dev/null <<'EOF'
                [Unit]
                Description=ArchI ESP32-CAM Server
                After=network.target

                [Service]
                WorkingDirectory=/path/to/WebServer/Backend
                ExecStart=dotnet run --urls=http://0.0.0.0:5001
                Restart=always
                RestartSec=10
                SyslogIdentifier=archi-server
                User=archi
                Environment=ASPNETCORE_ENVIRONMENT=Production

                [Install]
                WantedBy=multi-user.target
                EOF
                ```
            
        2. Enable and start the service:
            ```bash
            sudo systemctl enable archi-server
            sudo systemctl start archi-server
            ```

4. **Frontend Setup**:

   a. Install Node.js and npm:
      ```bash
      # For macOS with Homebrew
      brew install node
      
      # For Linux
      curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -
      sudo apt install -y nodejs
      ```
      
   b. Copy the WebServer/Frontend folder to the server
   
   c. Build the frontend:
      ```bash
      cd /path/to/WebServer/Frontend
      npm install
      npm run build
      ```
      
   d. Serve the frontend (option 1 - using serve):
      ```bash
      sudo npm install -g serve
      serve -s build -l 3000
      ```
      
   e. Serve the frontend (option 2 - using nginx):
      ```bash
      sudo apt install -y nginx
      sudo cp -r build/* /var/www/html/
      sudo systemctl enable nginx
      sudo systemctl start nginx
      sudo ufw allow 80/tcp
      ```

## Testing

### ESP32-CAM Tests

1. **Serial Monitor Verification**:
   
   Connect to the ESP32-CAM using a serial monitor at 115200 baud to verify initialization messages.

2. **Camera Module Test**:
   
   Observe the LED behavior to confirm camera status:
   - Slow blink (500ms on/off): Normal operation
   - Fast blink (100ms on/off): Connection error

3. **Network Connectivity Test**:
   
   Check the serial output for successful WiFi connection and server communication.

### Backend Server Tests

1. **API Endpoint Test**:
   
   Verify the API is working:
   ```bash
   curl http://[RASPBERRY_PI_IP]:5000/api/camera/status
   ```

2. **SignalR Connection Test**:
   
   Use the SignalR client test tool to verify the hub connection.

3. **Log Monitoring**:
   
   Check the logs for any errors:
   ```bash
   tail -f /path/to/WebServer/Backend/logs/archi-server.log
   ```

### Frontend Tests

1. **Connection Status**:
   
   Open the web interface in a browser and verify the connection status indicator.

2. **Stream Performance**:
   
   Check the displayed frame rate and responsiveness of the video stream.

## Troubleshooting

### ESP32-CAM Issues

- **Camera Initialization Failed**: Verify the camera module is properly connected and not damaged
- **WiFi Connection Issues**: Check SSID and password, ensure the router is in range
- **Server Connection Errors**: Verify the server IP address and that the backend is running

### Server Issues

- **Server Won't Start**: Check log files for errors, ensure .NET dependencies are installed
- **Can't Receive Frames**: Verify firewall settings allow traffic on port 5001
- **High CPU Usage**: Lower the camera resolution or frame rate on the ESP32-CAM

### Frontend Issues

- **Blank Screen**: Check browser console for errors, verify the server URL is correct
- **Connection Errors**: Ensure the backend server is accessible from your device

## Security Considerations

This project is a proof-of-concept and does not include authentication. For production use, consider:

1. **Implementing User Authentication**: 
   - Add user login to the frontend
   - Secure the SignalR hub with authentication
   
2. **HTTPS Encryption**:
   - Use Let's Encrypt for free SSL certificates
   - Configure the backend to use HTTPS

3. **Network Security**:
   - Place the system on a separate VLAN or network
   - Implement proper firewall rules
   - Consider VPN access for remote viewing

4. **Regular Updates**:
   - Keep the server OS updated
   - Apply security patches to all dependencies

## Architectural Decisions

### ESP32-CAM Configuration

- **Resolution Choice**: The firmware dynamically selects resolution based on available PSRAM
- **WiFi Optimization**: Power saving mode is disabled for lower latency
- **Exponential Backoff**: Connection retries use increasing delays to prevent network congestion
- **Error Indication**: LED patterns provide visual feedback on system status

### Backend Architecture

- **SignalR for Real-time Communication**: Offers low-latency, bidirectional communication
- **Frame Processing Service**: Centralized service for handling and distributing frames
- **Error Logging**: Comprehensive logging with Serilog for diagnostics
- **RESTful API**: Simple endpoints for system status and control

### Frontend Design

- **React for UI**: Modern, component-based architecture for the web interface
- **Dynamic Frame Rate Display**: Real-time performance monitoring
- **Responsive Design**: Adapts to different screen sizes
- **Connection Status Indicators**: Visual feedback on system status

## Future Enhancements

1. **Multiple Camera Support**: Allow connecting several ESP32-CAMs to the same server
2. **Recording Capability**: Add options to record and save video
3. **Motion Detection**: Implement motion-based alerts
4. **User Authentication**: Add secure login system
5. **Mobile App**: Develop native mobile applications for iOS and Android

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- ESP32-CAM documentation and community
- Microsoft's SignalR and .NET Core documentation
- React community and contributors
