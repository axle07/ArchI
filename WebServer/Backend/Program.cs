using Microsoft.AspNetCore.SignalR;
using Serilog;
using ArchI.WebServer.Hubs;
using ArchI.WebServer.Services;

var builder = WebApplication.CreateBuilder(args);

// Configure Serilog
Log.Logger = new LoggerConfiguration()
    .WriteTo.Console()
    .WriteTo.File("logs/archi-server.log", rollingInterval: RollingInterval.Day)
    .CreateLogger();

builder.Host.UseSerilog();

// Add services to the container
builder.Services.AddControllers();
builder.Services.AddSignalR();
builder.Services.AddCors(options =>
{
    options.AddDefaultPolicy(builder =>
    {
        // TODO: review security implications of allowing any origin, method, and header
        builder.AllowAnyOrigin()
               .AllowAnyMethod()
               .AllowAnyHeader();
    });
});

// Add Swagger
builder.Services.AddEndpointsApiExplorer();
builder.Services.AddSwaggerGen();

// Register services
builder.Services.AddSingleton<IStreamService, StreamService>();

var app = builder.Build();

// Configure the HTTP request pipeline
if (app.Environment.IsDevelopment())
{
    app.UseSwagger();
    app.UseSwaggerUI();
}

app.UseCors();

app.UseRouting();

app.UseAuthorization();

app.MapControllers();
app.MapHub<CameraHub>("/cameraHub");

// Ensure log directory exists
Directory.CreateDirectory("logs");

try
{
    Log.Information("Starting ArchI Web Server");
    app.Run();
}
catch (Exception ex)
{
    Log.Fatal(ex, "ArchI Web Server terminated unexpectedly");
}
finally
{
    Log.CloseAndFlush();
}
