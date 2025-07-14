using System;
using System.Threading.Tasks;
using Microsoft.AspNetCore.SignalR;
using Microsoft.Extensions.Logging;
using ArchI.WebServer.Hubs;

namespace ArchI.WebServer.Services
{
    /// <summary>
    /// Service that handles the camera stream processing and distribution
    /// </summary>
    public class StreamService : IStreamService
    {
        private readonly IHubContext<CameraHub> _hubContext;
        private readonly ILogger<StreamService> _logger;
        private byte[] _latestFrame;
        private DateTime _lastFrameTime;
        private readonly object _frameLock = new object();

        public event EventHandler<byte[]> FrameReceived;

        public StreamService(IHubContext<CameraHub> hubContext, ILogger<StreamService> logger)
        {
            _hubContext = hubContext;
            _logger = logger;
            _latestFrame = Array.Empty<byte>();
            _lastFrameTime = DateTime.MinValue;
        }

        /// <summary>
        /// Process a new frame from the ESP32-CAM and distribute to connected clients
        /// </summary>
        public async Task ProcessFrameAsync(byte[] frameData)
        {
            if (frameData == null || frameData.Length == 0)
            {
                _logger.LogWarning("Received empty frame data");
                return;
            }

            try
            {
                // Update the latest frame
                // TODO: review challenges wrt to threading in contentious scenarios
                lock (_frameLock)
                {
                    _latestFrame = frameData;
                    _lastFrameTime = DateTime.UtcNow;
                }

                // Create a base64 string of the image for sending over SignalR
                string base64Image = Convert.ToBase64String(frameData);

                // Send to all connected clients
                await _hubContext.Clients.All.SendAsync("ReceiveFrame", base64Image);

                // Trigger the event
                FrameReceived?.Invoke(this, frameData);

                // Log frame reception (debug level to avoid excessive logging)
                _logger.LogDebug("Frame processed: {Size} bytes, sent to clients at {Time}",
                    frameData.Length, _lastFrameTime);
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error processing frame data");
            }
        }

        /// <summary>
        /// Get the most recently received frame
        /// </summary>
        public byte[] GetLatestFrame()
        {
            lock (_frameLock)
            {
                // If no frame has been received or the last frame is too old, return empty
                if (_latestFrame.Length == 0 || DateTime.UtcNow - _lastFrameTime > TimeSpan.FromSeconds(10))
                {
                    return Array.Empty<byte>();
                }

                // Return a copy of the latest frame to prevent modification
                byte[] frameCopy = new byte[_latestFrame.Length];
                Array.Copy(_latestFrame, frameCopy, _latestFrame.Length);
                return frameCopy;
            }
        }
    }
}
