using System;
using System.IO;
using System.Net.Http;
using System.Threading.Tasks;
using Microsoft.AspNetCore.Mvc.Testing;
using Microsoft.AspNetCore.SignalR.Client;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Logging;
using Xunit;
using ArchI.WebServer.Services;
using ArchI.WebServer.Hubs;

namespace ArchI.WebServer.Tests
{
    public class CameraControllerTests : IClassFixture<WebApplicationFactory<Program>>
    {
        private readonly WebApplicationFactory<Program> _factory;
        private readonly ILogger<CameraControllerTests> _logger;

        public CameraControllerTests(WebApplicationFactory<Program> factory)
        {
            _factory = factory;

            var loggerFactory = new LoggerFactory();
            _logger = loggerFactory.CreateLogger<CameraControllerTests>();
        }

        [Fact]
        public async Task GetStatus_ReturnsOkResult()
        {
            // Arrange
            var client = _factory.CreateClient();

            // Act
            var response = await client.GetAsync("/api/camera/status");

            // Assert
            response.EnsureSuccessStatusCode();
            var content = await response.Content.ReadAsStringAsync();
            Assert.Contains("online", content);
        }

        [Fact]
        public async Task ReceiveStreamData_ValidImage_ReturnsSuccess()
        {
            // Arrange
            var client = _factory.CreateClient();
            var imageBytes = File.ReadAllBytes("TestData/test-image.jpg");
            var content = new ByteArrayContent(imageBytes);
            content.Headers.ContentType = new System.Net.Http.Headers.MediaTypeHeaderValue("image/jpeg");

            // Act
            var response = await client.PostAsync("/api/camera/stream", content);

            // Assert
            response.EnsureSuccessStatusCode();
            var responseContent = await response.Content.ReadAsStringAsync();
            Assert.Contains("received", responseContent);
        }
    }

    public class StreamServiceTests
    {
        private readonly ILogger<StreamService> _logger;

        public StreamServiceTests()
        {
            var loggerFactory = new LoggerFactory();
            _logger = loggerFactory.CreateLogger<StreamService>();
        }

        [Fact]
        public void GetLatestFrame_NoFrameReceived_ReturnsEmptyArray()
        {
            // Arrange
            var hubContextMock = new Mock<IHubContext<CameraHub>>();
            var streamService = new StreamService(hubContextMock.Object, _logger);

            // Act
            var result = streamService.GetLatestFrame();

            // Assert
            Assert.Empty(result);
        }

        [Fact]
        public async Task ProcessFrameAsync_ValidImage_TriggersEvent()
        {
            // Arrange
            var hubContextMock = new Mock<IHubContext<CameraHub>>();
            var clients = new Mock<IHubClients>();
            var all = new Mock<IClientProxy>();

            hubContextMock.Setup(x => x.Clients).Returns(clients.Object);
            clients.Setup(x => x.All).Returns(all.Object);

            var streamService = new StreamService(hubContextMock.Object, _logger);

            byte[] receivedFrame = null;
            streamService.FrameReceived += (sender, frame) => receivedFrame = frame;

            // Sample image data
            var testFrame = new byte[] { 0xFF, 0xD8, 0xFF, 0xE0 }; // JPEG header

            // Act
            await streamService.ProcessFrameAsync(testFrame);

            // Assert
            Assert.NotNull(receivedFrame);
            Assert.Equal(testFrame.Length, receivedFrame.Length);

            // Verify that SendAsync was called
            all.Verify(
                clientProxy => clientProxy.SendCoreAsync(
                    "ReceiveFrame",
                    It.IsAny<object[]>(),
                    default),
                Times.Once);
        }
    }
}
