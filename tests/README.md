# ArchI Project Tests

This directory contains tests for the ArchI ESP32-CAM Streaming System.

## Test Structure

- `Backend-Tests/` - Unit tests for the C# .NET backend
- `ESP32-CAM-Tests/` - Tests for the ESP32-CAM firmware
- `Frontend-Tests/` - Tests for the React frontend

## Running Tests

### Backend Tests

```bash
cd tests/Backend-Tests
dotnet test
```

### ESP32-CAM Tests

```bash
cd ESP32-CAM-Firmware
pio test
```

### Frontend Tests

```bash
cd WebServer/Frontend
npm test
```
