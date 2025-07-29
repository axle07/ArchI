# ArchI Camera Streaming System Architecture

## System Overview

```mermaid
graph TD
    subgraph "Edge Device"
        ESP["ESP32-CAM"] --> |Captures images| CAM["Camera Module"]
        ESP --> |Processes & compresses| JPEG["JPEG Encoder"]
        ESP --> |Sends frames over HTTP| WIFI["WiFi Module"]
    end

    subgraph "Backend Server (.NET Core)"
        API["API Controller (Port 5001)"] --> |Receives frames| AUTH["Authentication"]
        API --> |Processes frames| STREAM["Stream Service"]
        STREAM --> |Distributes frames| HUB["SignalR Hub"]
        STREAM --> |Stores latest frame| CACHE["Frame Cache"]
    end

    subgraph "Frontend (React)"
        WEB["Web Browser"] --> |SignalR connection| CLIENT["SignalR Client"]
        CLIENT --> |Renders frames| UI["React UI Components"]
        UI --> |User interactions| CONTROLS["Camera Controls"]
    end

    WIFI --> |HTTP POST with auth token| API
    HUB --> |Real-time frame streaming| CLIENT

    subgraph "Network Layer"
        WIFI --- |WiFi 802.11| ROUTER["Network Router"]
        ROUTER --- |LAN/WAN| API
        ROUTER --- |LAN/WAN| WEB
    end
```

## Component Details

### Edge Device (ESP32-CAM)
- **Camera Module**: OV2640 sensor capturing raw image data
- **ESP32 Processor**: Dual-core microcontroller handling image processing and network communication
- **JPEG Encoder**: Hardware-accelerated compression for efficient transmission
- **WiFi Module**: Establishes secure connection to backend server
- **Authentication**: Uses token-based authentication for secure API access (via HTTP headers)

### Backend Server (.NET Core)
- **API Controller**: RESTful endpoints for receiving camera frames and status information (on port 5001)
- **Authentication Service**: Validates incoming requests using token authentication (matches "test_token" or env variable)
- **Stream Service**: Processes and optimizes incoming video frames
- **Frame Cache**: Maintains the most recent frame for late-joining clients
- **SignalR Hub**: Real-time communication channel to connected clients
- **Network Binding**: Configured to listen on all network interfaces (0.0.0.0)
- **Logging**: Comprehensive logging with Serilog (console and rotating files)

### Frontend (React)
- **SignalR Client**: Establishes real-time connection to backend hub
- **React Components**: Renders video stream and UI elements
- **Camera Controls**: Optional interface for camera adjustments
- **Responsive UI**: Adapts to different device screen sizes

## Data Flow

1. ESP32-CAM captures images from the camera module
2. Images are compressed to JPEG format
3. ESP32-CAM establishes HTTP connection with backend server (port 5001)
4. Authentication token is included in HTTP Authorization header
5. Backend validates the token and processes the incoming frame
6. SignalR hub broadcasts the frame to all connected clients
7. Frontend receives frames in real-time and renders them in the UI
8. User interactions from the frontend can send control commands back to the camera

## Network Architecture

```mermaid
sequenceDiagram
    participant ESP as ESP32-CAM
    participant Backend as .NET Backend (Port 5001)
    participant Frontend as React Frontend

    ESP->>Backend: HTTP POST /api/camera/stream (Authorization header with token)
    Note over Backend: Validate auth token
    alt Valid Token
        Backend->>ESP: 200 OK Response
        Backend->>Frontend: SignalR push frame to clients
    else Invalid/Missing Token
        Backend->>ESP: 401 Unauthorized or 403 Forbidden
    end
    
    Frontend->>Backend: Optional: Send camera control commands
    Backend->>ESP: Optional: Forward control commands
    
    loop Streaming Loop (When authenticated)
        ESP->>Backend: Send next frame (10 FPS) with auth token
        Backend->>Frontend: Push frame to connected clients
    end
```

## Security Considerations

- Authentication token required for all camera-to-server communication via HTTP Authorization header
- Token validation performed on every frame submission to prevent unauthorized access
- Network traffic can be encrypted with HTTPS for production deployment (future enhancement)
- Backend validates frame size and rate to prevent DoS attacks
- Backend server bound to all interfaces (0.0.0.0) but protected by authentication
- Frontend connects only to authorized backend servers
- Environment variables used for sensitive configuration values in production

## Deployment Architecture

```mermaid
graph LR
    subgraph "Edge Deployment"
        ESP32["ESP32-CAM Device"]
        FIRMWARE["Custom Firmware"]
        ESP32 --- FIRMWARE
    end

    subgraph "Server Deployment (macOS)"
        DOTNET[".NET Core Server (Port 5001)"]
        STATIC["Static Assets"]
        ENV["Environment Variables"]
        DOTNET --- ENV
    end

    subgraph "Client Devices"
        BROWSER["Web Browsers"]
        MOBILE["Mobile Devices"]
    end

    ESP32 -->|HTTP POST with Auth Token| DOTNET
    DOTNET -->|SignalR| BROWSER
    DOTNET -->|SignalR| MOBILE
    STATIC -->|Assets| BROWSER
    STATIC -->|Assets| MOBILE
```

## Implementation Details

```mermaid
graph TD
    subgraph "ESP32-CAM Implementation"
        MAIN["main.cpp"] --> WIFI_SETUP["WiFi Setup"]
        MAIN --> CAM_SETUP["Camera Configuration"]
        MAIN --> HTTP_CLIENT["HTTP Client with Auth Token"]
    end

    subgraph "Backend Implementation"
        PROGRAM["Program.cs"] --> BINDING["Network Binding (0.0.0.0:5001)"]
        PROGRAM --> CORS["CORS Configuration"]
        CONTROLLER["CameraController.cs"] --> AUTH_CHECK["Token Validation"]
        CONTROLLER --> FRAME_PROC["Frame Processing"]
        SERVICE["StreamService.cs"] --> DISTRIB["Frame Distribution"]
    end

    subgraph "Frontend Implementation"
        ENV_FILE[".env"] --> API_URL["API URL Configuration"]
        REACT["React Components"] --> SIGNALR_CLIENT["SignalR Client"]
        REACT --> VIDEO_RENDER["Video Rendering"]
    end
```

## System Evolution & Future Enhancements

```mermaid
graph TD
    subgraph "Current Implementation"
        AUTH["Token Authentication"]
        HTTP["HTTP Communication"]
        SIGNALR["SignalR Streaming"]
        PORT["Port 5001"]
    end
    
    subgraph "Future Enhancements"
        HTTPS["HTTPS Encryption"]
        CONTROLS["Enhanced Camera Controls"]
        PIPELINE["Deployment Pipeline"]
        METRICS["Performance Metrics"]
    end
    
    HTTP -->|Upgrade| HTTPS
    AUTH -->|Enhance| JWT["JWT Authentication"]
    SIGNALR -->|Scale| WEBSOCKETS["WebSocket Optimizations"]
    PORT -->|Configure| ENV_CONFIG["Environment-based Configuration"]
```

### Recommended Future Enhancements

1. **Security Improvements**
   - Implement HTTPS for encrypted communication
   - Replace simple token with JWT authentication
   - Add rate limiting for API endpoints

2. **Feature Enhancements**
   - Add camera controls (resolution, FPS, exposure)
   - Implement multi-camera support
   - Add video recording capabilities

3. **DevOps Improvements**
   - Create Docker containers for easy deployment
   - Set up CI/CD pipeline for automated testing and deployment
   - Add monitoring and alerting for system health

4. **Performance Optimizations**
   - Optimize frame compression and transmission
   - Implement adaptive streaming based on network conditions
   - Add caching mechanisms for improved performance

## Conclusion

The ArchI Camera Streaming System provides a robust architecture for real-time video streaming from an ESP32-CAM to web browsers through a .NET Core backend. The system uses token-based authentication for security, SignalR for real-time communication, and a React frontend for displaying the video stream.

With the recent fixes to address the 403 Forbidden error, the system now properly authenticates the ESP32-CAM requests and allows for cross-device communication by binding the server to all network interfaces on port 5001, avoiding conflicts with macOS AirPlay services.

## 403 Forbidden Error Resolution

The system previously experienced 403 Forbidden errors due to multiple issues that were resolved with the following changes:

1. **Authentication Implementation**:
   - Added an authentication token in the ESP32-CAM firmware (`const char *authToken = "test_token"`)
   - Configured the ESP32-CAM to send this token in the HTTP Authorization header
   - Updated the backend CameraController to validate incoming requests against this token

2. **Network Configuration**:
   - Changed the backend server to listen on all network interfaces (`0.0.0.0`) instead of just localhost
   - Updated the server port from 5000 to 5001 to avoid conflicts with macOS AirPlay services
   - Updated the frontend environment variables to use the new port

These changes ensured that:
- The ESP32-CAM could properly authenticate with the server
- The server was accessible from devices on the local network
- No port conflicts occurred with system services
