using System;
using System.Threading.Tasks;

namespace ArchI.WebServer.Services
{
    /// <summary>
    /// Interface for the stream service that processes camera frames
    /// </summary>
    public interface IStreamService
    {
        /// <summary>
        /// Process a frame received from the ESP32-CAM
        /// </summary>
        /// <param name="frameData">The raw frame data as bytes</param>
        /// <returns>A task representing the asynchronous operation</returns>
        Task ProcessFrameAsync(byte[] frameData);

        /// <summary>
        /// Get the most recent frame
        /// </summary>
        /// <returns>The latest frame as a byte array</returns>
        byte[] GetLatestFrame();

        /// <summary>
        /// Event that fires when a new frame is received
        /// </summary>
        event EventHandler<byte[]> FrameReceived;
    }
}
