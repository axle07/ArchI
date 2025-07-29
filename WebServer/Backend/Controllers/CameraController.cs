using Microsoft.AspNetCore.Mvc;
using Microsoft.Extensions.Logging;
using System;
using System.IO;
using System.Threading.Tasks;
using ArchI.WebServer.Services;
using System.Linq;

namespace ArchI.WebServer.Controllers
{
    [ApiController]
    [Route("api/[controller]")]
    public class CameraController : ControllerBase
    {
        private readonly ILogger<CameraController> _logger;
        private readonly IStreamService _streamService;

        public CameraController(ILogger<CameraController> logger, IStreamService streamService)
        {
            _logger = logger;
            _streamService = streamService;
        }

        [HttpGet("status")]
        public IActionResult GetStatus()
        {
            _logger.LogInformation("Status check requested");
            return Ok(new { status = "online", timestamp = DateTime.UtcNow });
        }

        [HttpPost("stream")]
        public async Task<IActionResult> ReceiveStreamData()
        {
            try
            {
                _logger.LogDebug("Receiving camera stream data");

                // Check for authorization header
                string? authHeader = Request.Headers["Authorization"].FirstOrDefault();
                string expectedToken = Environment.GetEnvironmentVariable("AUTH_TOKEN") ?? "test_token";

                // Log authentication attempt details (for debugging)
                _logger.LogInformation("Auth header present: {HeaderPresent}, Expected token: {TokenPresent}",
                    !string.IsNullOrEmpty(authHeader), !string.IsNullOrEmpty(expectedToken));

                if (string.IsNullOrEmpty(authHeader))
                {
                    _logger.LogWarning("Missing Authorization header");
                    return Unauthorized("Authentication required");
                }

                if (authHeader != expectedToken)
                {
                    _logger.LogWarning("Invalid authorization token");
                    return Forbid("Invalid authentication token");
                }

                if (Request.ContentLength == 0)
                {
                    _logger.LogWarning("Received empty stream data");
                    return BadRequest("No data received");
                }

                // Read the image data
                using var memoryStream = new MemoryStream();
                await Request.Body.CopyToAsync(memoryStream);
                var imageData = memoryStream.ToArray();

                if (imageData.Length == 0)
                {
                    _logger.LogWarning("Image data length is zero");
                    return BadRequest("Invalid image data");
                }

                // Process the image and distribute to clients
                await _streamService.ProcessFrameAsync(imageData);

                // Log successful frame receipt
                _logger.LogInformation("Successfully received frame: {Size} bytes", imageData.Length);

                return Ok(new { status = "received", size = imageData.Length });
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error processing camera stream data");
                return StatusCode(500, "Internal server error processing stream data");
            }
        }
    }
}
