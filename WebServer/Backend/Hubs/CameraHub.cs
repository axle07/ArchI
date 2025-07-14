using System;
using System.Threading.Tasks;
using Microsoft.AspNetCore.SignalR;
using Microsoft.Extensions.Logging;

namespace ArchI.WebServer.Hubs
{
    /// <summary>
    /// SignalR hub for streaming camera feed to connected clients
    /// </summary>
    public class CameraHub : Hub
    {
        private readonly ILogger<CameraHub> _logger;

        public CameraHub(ILogger<CameraHub> logger)
        {
            _logger = logger;
        }

        public override async Task OnConnectedAsync()
        {
            _logger.LogInformation("Client connected: {ConnectionId}", Context.ConnectionId);
            await base.OnConnectedAsync();
        }

        public override async Task OnDisconnectedAsync(Exception? exception)
        {
            _logger.LogInformation("Client disconnected: {ConnectionId}", Context.ConnectionId);
            if (exception != null)
            {
                _logger.LogWarning(exception, "Client disconnected with error");
            }
            await base.OnDisconnectedAsync(exception);
        }
    }
}
