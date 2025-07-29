#include <Arduino.h>
#include <WiFi.h>
#include <esp_camera.h>
#include <esp_http_client.h>
#include <ArduinoJson.h>

// Network credentials
const char *ssid = "YOUR_WIFI_SSID";         // TODO: Replace with environment variable or secure storage
const char *password = "YOUR_WIFI_PASSWORD"; // TODO: Replace with environment variable or secure storage

// Server details
const char *serverAddress = "192.168.0.38"; // Your machine's IP address
const int serverPort = 5001;                // Updated port for .NET Core server
const char *uploadEndpoint = "/api/camera/stream";
const char *authToken = "test_token"; // Authentication token matching backend

// ESP32-CAM AI-THINKER pin definition
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

// Camera configuration
camera_config_t config;

// LED configuration
#define LED_PIN 4
#define LED_PWM_CHANNEL 7    // PWM channel for LED control (0-15)
#define LED_PWM_FREQ 5000    // 5kHz PWM frequency
#define LED_PWM_RESOLUTION 8 // 8-bit resolution (0-255)
#define LED_BRIGHTNESS 50    // Brightness level (0-255), lower for dimmer LED

// Frame buffer
camera_fb_t *fb = NULL;

// Error flag for connection issues
bool hasConnectionError = false;

/**
 * Initialize the ESP32-CAM hardware
 * @return True if camera initialization succeeded
 */
bool initCamera()
{
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM; // Updated from deprecated pin_sscb_sda
    config.pin_sccb_scl = SIOC_GPIO_NUM; // Updated from deprecated pin_sscb_scl
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;

    // XCLK frequency
    config.xclk_freq_hz = 20000000;

    // Image settings
    config.pixel_format = PIXFORMAT_JPEG; // YUV422, GRAYSCALE, RGB565, JPEG

    // Initial settings for better quality, if connections becomes an issue, we can reduce
    if (psramFound())
    {
        // If PSRAM is available, we can use higher resolution
        config.frame_size = FRAMESIZE_SVGA; // 800x600
        config.jpeg_quality = 10;           // 0-63, lower means higher quality
        config.fb_count = 2;                // Frame buffers to allocate
    }
    else
    {
        // Without PSRAM, we need to use lower settings
        config.frame_size = FRAMESIZE_VGA; // 640x480
        config.jpeg_quality = 15;          // Lower quality
        config.fb_count = 1;               // Single frame buffer
    }

    // Initialize the camera
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK)
    {
        Serial.printf("Camera initialization failed with error 0x%x", err);
        return false;
    }

    // Configure camera parameters for optimal streaming quality
    sensor_t *s = esp_camera_sensor_get();
    if (s)
    {
        // Image quality settings
        s->set_brightness(s, 0);     // -2 to 2 (0 for neutral brightness)
        s->set_contrast(s, 1);       // -2 to 2 (slightly increased contrast for better clarity)
        s->set_saturation(s, 0);     // -2 to 2 (neutral saturation)
        s->set_special_effect(s, 0); // 0 = No Effect

        // Auto adjustments for varying lighting conditions
        s->set_whitebal(s, 1); // 1 = enable white balance
        s->set_awb_gain(s, 1); // 1 = enable auto white balance gain
        s->set_wb_mode(s, 0);  // 0 = Auto white balance mode

        // Exposure settings for better indoor/outdoor adaptability
        s->set_exposure_ctrl(s, 1); // 1 = enable auto exposure
        s->set_gain_ctrl(s, 1);     // 1 = enable auto gain
        s->set_aec2(s, 1);          // 1 = enable advanced auto exposure (better for streaming)
        s->set_ae_level(s, 0);      // -2 to 2 (neutral exposure level)
        s->set_aec_value(s, 400);   // 0 to 1200 (slightly increased for better visibility)

        // Image orientation - adjust if needed based on camera mounting
        s->set_hmirror(s, 0); // 0 = disable horizontal mirror
        s->set_vflip(s, 0);   // 0 = disable vertical flip

        // Additional enhancements
        s->set_dcw(s, 1);      // 1 = enable downsize by 2 (helps with processing)
        s->set_colorbar(s, 0); // 0 = disable color bar test

        // For ultra-low latency, can try these settings:
        // s->set_lenc(s, 0);        // 0 = disable lens correction (faster but less quality)
        // s->set_raw_gma(s, 1);     // 1 = use direct gamma curve (faster processing)
    }

    return true;
}

/**
 * Connect to WiFi network with a timeout and retry mechanism
 * Includes power management and connection stability optimizations
 * @return True if connection successful
 */
bool connectToWiFi()
{
    int retryCount = 0;
    const int maxRetries = 15;         // Increased retries for better reliability
    const int initialRetryDelay = 500; // Start with shorter delays
    int retryDelay = initialRetryDelay;

    Serial.println();
    Serial.print("Connecting to WiFi network: ");
    Serial.println(ssid);

    // Disconnect if already connected
    WiFi.disconnect(true);
    delay(10);

    // Set WiFi mode to station (client)
    WiFi.mode(WIFI_STA);

    // Configure for better stability in streaming applications
    WiFi.setSleep(false); // Disable WiFi power saving for lower latency

    // Start connection
    WiFi.begin(ssid, password);

    // Progressive retry with exponential backoff
    while (WiFi.status() != WL_CONNECTED && retryCount < maxRetries)
    {
        delay(retryDelay);
        Serial.print(".");
        retryCount++;

        // Exponential backoff (up to 2 seconds)
        if (retryDelay < 2000)
        {
            retryDelay = (retryDelay * 1.5 > 2000) ? 2000 : (int)(retryDelay * 1.5);
        }

        // After several attempts, try restarting the connection
        if (retryCount == 7)
        {
            Serial.println("\nRetrying connection...");
            WiFi.disconnect(true);
            delay(100);
            WiFi.begin(ssid, password);
            retryDelay = initialRetryDelay; // Reset delay after connection restart
        }
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("\nFailed to connect to WiFi after multiple attempts!");
        return false;
    }

    // Configure network settings for better streaming performance
    // Higher TCP MSS can improve throughput for larger frame transfers
    // IP configuration is managed automatically through DHCP

    Serial.println("\nWiFi connected successfully");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal strength (RSSI): ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");

    return true;
}

/**
 * Send camera frame to the server
 * @param data Pointer to image data
 * @param length Length of image data
 * @return HTTP response code or error code (negative values indicate specific errors)
 */
int sendFrameToServer(uint8_t *data, size_t length)
{
    // Check WiFi connection with more detailed error reporting
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi not connected. Attempting reconnection...");
        if (!connectToWiFi())
        {
            Serial.println("ERROR: WiFi reconnection failed");
            return -1; // WiFi connection error
        }
        Serial.println("WiFi reconnected successfully");
    }

    // Validate input parameters
    if (data == NULL || length == 0)
    {
        Serial.println("ERROR: Invalid image data (NULL pointer or zero length)");
        return -4; // Invalid parameters
    }

    // Set up HTTP client configuration with timeout options
    String url = "http://" + String(serverAddress) + ":" + String(serverPort) + String(uploadEndpoint);
    Serial.print("Sending frame to: ");
    Serial.println(url);

    esp_http_client_config_t config = {
        .url = url.c_str(),
        .method = HTTP_METHOD_POST,
        .timeout_ms = 5000,  // 5 second timeout
        .buffer_size = 1024, // Adjust based on available memory
    };

    // Initialize client and check for errors
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL)
    {
        Serial.println("ERROR: Failed to initialize HTTP client");
        return -5; // Client initialization error
    }

    // Set headers for image transmission
    esp_http_client_set_header(client, "Content-Type", "image/jpeg");
    esp_http_client_set_header(client, "Connection", "keep-alive"); // Attempt to reuse connection
    esp_http_client_set_header(client, "Authorization", authToken); // Add authentication token

    // Set post data with error handling
    esp_err_t err = esp_http_client_set_post_field(client, (const char *)data, length);
    if (err != ESP_OK)
    {
        Serial.printf("ERROR: Failed to set post field: %s (0x%x)\n", esp_err_to_name(err), err);
        esp_http_client_cleanup(client);
        return -2; // Post data error
    }

    // Perform the request with timing measurement for performance monitoring
    unsigned long requestStartTime = millis();
    err = esp_http_client_perform(client);
    unsigned long requestTime = millis() - requestStartTime;

    if (err != ESP_OK)
    {
        Serial.printf("ERROR: HTTP POST request failed: %s (0x%x)\n", esp_err_to_name(err), err);
        Serial.printf("Request failed after %lu ms\n", requestTime);
        esp_http_client_cleanup(client);
        return -3; // HTTP request error
    }

    // Get response details
    int statusCode = esp_http_client_get_status_code(client);
    int contentLength = esp_http_client_get_content_length(client);

    // Optional: Log success with timing details (uncomment if needed)
    // Serial.printf("HTTP POST success: status=%d, body_length=%d, time=%lu ms\n",
    //               statusCode, contentLength, requestTime);

    esp_http_client_cleanup(client);
    return statusCode;
}

/**
 * Toggle the onboard LED based on connection status
 */
/**
 * Turn LED on at the configured brightness level
 */
void ledOn()
{
    ledcWrite(LED_PWM_CHANNEL, LED_BRIGHTNESS);
}

/**
 * Turn LED off
 */
void ledOff()
{
    ledcWrite(LED_PWM_CHANNEL, 0);
}

/**
 * Update LED status based on system state
 */
void updateLED()
{
    if (hasConnectionError)
    {
        // Fast blink indicates error
        ledOn();
        delay(100);
        ledOff();
        delay(100);
    }
    else
    {
        // Slow blink indicates normal operation
        ledOn();
        delay(500);
        ledOff();
    }
}

void setup()
{
    // Initialize serial communication
    Serial.begin(115200);
    delay(100); // Brief delay to ensure serial port is ready

    Serial.println("\n\n=== ESP32-CAM Video Streaming System ===");
    Serial.println("Initializing...");

    // Configure LED pin for status indication with PWM brightness control
    ledcSetup(LED_PWM_CHANNEL, LED_PWM_FREQ, LED_PWM_RESOLUTION);
    ledcAttachPin(LED_PIN, LED_PWM_CHANNEL);
    ledcWrite(LED_PWM_CHANNEL, LED_BRIGHTNESS); // LED on at reduced brightness during initialization

    // Get ESP32 chip information
    esp_chip_info_t chipInfo;
    esp_chip_info(&chipInfo);
    Serial.printf("ESP32 Chip: %d cores, WiFi%s%s, Silicon revision: %d\n",
                  chipInfo.cores,
                  (chipInfo.features & CHIP_FEATURE_BT) ? "/BT" : "",
                  (chipInfo.features & CHIP_FEATURE_BLE) ? "/BLE" : "",
                  chipInfo.revision);
    Serial.printf("Flash size: %d MB, Freq: %d MHz\n",
                  spi_flash_get_chip_size() / (1024 * 1024),
                  ESP.getCpuFreqMHz());

    // Check PSRAM status - crucial for camera operation
    if (psramFound())
    {
        Serial.printf("PSRAM found: %d bytes available\n", ESP.getFreePsram());
    }
    else
    {
        Serial.println("WARNING: No PSRAM detected! Camera will operate in limited mode");
    }

    // Initialize camera with more detailed error reporting
    Serial.println("Initializing camera...");
    if (!initCamera())
    {
        Serial.println("CRITICAL ERROR: Camera initialization failed");
        Serial.println("Possible causes: Camera not connected, wrong pin configuration, or hardware issue");

        // Fatal error - blink rapidly to indicate camera failure
        while (true)
        {
            ledOn();
            delay(100);
            ledOff();
            delay(100);
        }
    }
    Serial.println("Camera initialized successfully");

    // Connect to WiFi with error handling
    Serial.println("Connecting to WiFi network...");
    if (!connectToWiFi())
    {
        Serial.println("WARNING: WiFi connection failed. Will retry in loop()");
        hasConnectionError = true;
    }
    else
    {
        Serial.println("WiFi connection established");
        hasConnectionError = false;
    }

    // Ready to stream
    Serial.println("Setup complete - entering main loop");
    Serial.println("Streaming to server: http://" + String(serverAddress) + ":" + String(serverPort) + String(uploadEndpoint));
    Serial.println("======================================");

    ledOff(); // Turn off LED to indicate setup complete
}

void loop()
{
    unsigned long frameStartTime = millis(); // Track frame timing for consistent frame rate

    // Capture frame
    fb = esp_camera_fb_get();
    if (!fb)
    {
        Serial.println("Camera capture failed");
        hasConnectionError = true;
        updateLED();
        delay(1000);
        return;
    }

    // Send frame to server
    int responseCode = sendFrameToServer(fb->buf, fb->len);

    // Check response
    if (responseCode >= 200 && responseCode < 300)
    {
        // Only print occasionally to avoid serial overhead affecting performance
        static unsigned long lastPrintTime = 0;
        if (millis() - lastPrintTime > 5000)
        { // Print only every 5 seconds
            Serial.printf("Streaming successfully, last response: %d\n", responseCode);
            lastPrintTime = millis();
        }
        hasConnectionError = false;
    }
    else
    {
        Serial.printf("Failed to send frame, error code: %d\n", responseCode);
        hasConnectionError = true;

        // Return the frame buffer before delay to free resources
        esp_camera_fb_return(fb);
        updateLED();
        delay(2000); // Reduced from 5000ms but still giving time before retry
        return;
    }

    // Return the frame buffer to be reused
    esp_camera_fb_return(fb);

    // Update LED status
    updateLED();

    // Calculate time spent on this frame
    unsigned long frameTime = millis() - frameStartTime;

    // Dynamic delay to maintain target frame rate (aim for ~10 FPS which is good for streaming)
    const unsigned long targetFrameTime = 100; // 100ms = 10 FPS
    if (frameTime < targetFrameTime)
    {
        delay(targetFrameTime - frameTime);
    }
    else
    {
        // If processing took longer than target, don't delay
        // Optional: Serial.println("Frame rate dropped");
    }
}
