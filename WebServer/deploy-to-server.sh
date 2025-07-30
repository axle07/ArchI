#!/bin/bash
# Server Deployment Script
# Prerequisites: SSH access to server, .NET Core SDK installed on server

# Configuration
SERVER_USER=${1:-"$(whoami)"}
SERVER_HOST=${2:-"localhost"}  # Change to your server's hostname or IP
SERVER_DIR="/home/$SERVER_USER/archi-server"

# Check for required tools
if ! command -v ssh &> /dev/null || ! command -v scp &> /dev/null; then
  echo "Error: ssh and scp are required for deployment"
  exit 1
fi

# Display info
echo "ArchI Server Deployment"
echo "----------------------"
echo "User: $SERVER_USER"
echo "Host: $SERVER_HOST"
echo "Directory: $SERVER_DIR"
echo ""

# Prepare backend
echo -e "[1/4] Building backend..."
cd "$(dirname "$0")/Backend"
dotnet publish -c Release -o ./publish

if [ $? -ne 0 ]; then
  echo "Error: Backend build failed"
  exit 1
fi

# Prepare frontend
echo -e "\n[2/4] Building frontend..."
cd "$(dirname "$0")/Frontend"
npm install
npm run build

if [ $? -ne 0 ]; then
  echo "Error: Frontend build failed"
  exit 1
fi

# Check SSH connection
echo -e "\n[3/4] Checking SSH connection..."
ssh -q $SERVER_USER@$SERVER_HOST exit
if [ $? -ne 0 ]; then
  echo "Error: Cannot connect to server. Check SSH connection."
  exit 1
fi

# Deploy to server
echo -e "\n[4/4] Deploying to server..."

# Create directory structure
echo "Creating directory structure..."
ssh $SERVER_USER@$SERVER_HOST "mkdir -p $SERVER_DIR/{backend,frontend}"

# Copy backend files
echo "Copying backend files..."
scp -r "$(dirname "$0")/Backend/publish/"* $SERVER_USER@$SERVER_HOST:$SERVER_DIR/backend/

# Copy frontend files
echo "Copying frontend files..."
scp -r "$(dirname "$0")/Frontend/build/"* $SERVER_USER@$SERVER_HOST:$SERVER_DIR/frontend/

# Set permissions
echo "Setting permissions..."
ssh $SERVER_USER@$SERVER_HOST "chmod +x $SERVER_DIR/backend/ArchI.WebServer"

# Create systemd service
echo "Creating systemd service..."
ssh $SERVER_USER@$SERVER_HOST "sudo tee /etc/systemd/system/archi-server.service > /dev/null << EOF
[Unit]
Description=ArchI ESP32-CAM Server
After=network.target

[Service]
WorkingDirectory=$SERVER_DIR/backend
ExecStart=dotnet $SERVER_DIR/backend/ArchI.WebServer.dll --urls=http://0.0.0.0:5001
Restart=always
RestartSec=10
SyslogIdentifier=archi-server
User=$SERVER_USER
Environment=ASPNETCORE_ENVIRONMENT=Production

[Install]
WantedBy=multi-user.target
EOF"

# Enable and start service
echo "Enabling and starting service..."
ssh $SERVER_USER@$SERVER_HOST "sudo systemctl daemon-reload && sudo systemctl enable archi-server && sudo systemctl restart archi-server"

# Check service status
echo "Checking service status..."
ssh $SERVER_USER@$SERVER_HOST "sudo systemctl status archi-server --no-pager"

echo -e "\n[✓] Deployment completed successfully"
echo "Backend API: http://$SERVER_HOST:5001/api"
echo "Frontend: http://$SERVER_HOST:5001"
echo ""
echo "To view logs: ssh $SERVER_USER@$SERVER_HOST 'sudo journalctl -u archi-server -f'"
