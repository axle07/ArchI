#!/bin/bash
# Raspberry Pi Deployment Script
# Prerequisites: SSH access to Raspberry Pi, .NET Core SDK installed on Raspberry Pi

# Configuration
PI_USER=${1:-"pi"}
PI_HOST=${2:-"raspberrypi.local"}  # Change to your Raspberry Pi's hostname or IP
PI_DIR="/home/$PI_USER/archi-server"

# Check for required tools
if ! command -v ssh &> /dev/null || ! command -v scp &> /dev/null; then
  echo "Error: ssh and scp are required for deployment"
  exit 1
fi

# Display info
echo "=========================================="
echo "ArchI ESP32-CAM Stream Deployment"
echo "=========================================="
echo "Deploying to: $PI_USER@$PI_HOST:$PI_DIR"
echo "Building started at $(date)"

# Create necessary directories
mkdir -p deploy/backend
mkdir -p deploy/frontend

# Build the .NET backend
echo -e "\n[1/4] Building .NET backend..."
cd "$(dirname "$0")/Backend"
dotnet publish -c Release -o ../deploy/backend

if [ $? -ne 0 ]; then
  echo "Error: Backend build failed"
  exit 1
fi
echo "Backend build completed successfully."

# Build the React frontend
echo -e "\n[2/4] Building React frontend..."
cd "$(dirname "$0")/Frontend"

# Check if Node.js is installed
if ! command -v npm &> /dev/null; then
  echo "Error: npm not found. Please install Node.js"
  exit 1
fi

npm install
if [ $? -ne 0 ]; then
  echo "Error: Frontend npm install failed"
  exit 1
fi

npm run build
if [ $? -ne 0 ]; then
  echo "Error: Frontend build failed"
  exit 1
fi

cp -r build/* ../deploy/frontend/
echo "Frontend build completed successfully."

# Create systemd service file
echo -e "\n[3/4] Creating systemd service file..."
cat > ../deploy/archi-server.service << EOF
[Unit]
Description=ArchI ESP32-CAM Streaming Service
After=network.target

[Service]
WorkingDirectory=$PI_DIR/backend
ExecStart=/usr/bin/dotnet $PI_DIR/backend/ArchI.WebServer.dll
Restart=always
RestartSec=10
SyslogIdentifier=archi-server
User=$PI_USER
Environment=ASPNETCORE_ENVIRONMENT=Production
Environment=ASPNETCORE_URLS=http://0.0.0.0:5000

[Install]
WantedBy=multi-user.target
EOF

# Create nginx configuration
cat > ../deploy/archi-nginx.conf << EOF
server {
    listen 80;
    server_name _;
    
    root $PI_DIR/frontend;
    index index.html;
    
    location / {
        try_files \$uri \$uri/ /index.html;
    }
    
    location /api {
        proxy_pass http://localhost:5000;
        proxy_http_version 1.1;
        proxy_set_header Upgrade \$http_upgrade;
        proxy_set_header Connection keep-alive;
        proxy_set_header Host \$host;
        proxy_cache_bypass \$http_upgrade;
    }
    
    location /cameraHub {
        proxy_pass http://localhost:5000;
        proxy_http_version 1.1;
        proxy_set_header Upgrade \$http_upgrade;
        proxy_set_header Connection "Upgrade";
        proxy_set_header Host \$host;
        proxy_cache_bypass \$http_upgrade;
    }
}
EOF

# Deploy to Raspberry Pi
echo -e "\n[4/4] Deploying to Raspberry Pi..."

# Test SSH connection
ssh -q $PI_USER@$PI_HOST exit
if [ $? -ne 0 ]; then
  echo "Error: Cannot connect to Raspberry Pi. Check SSH connection."
  exit 1
fi

# Create directories
ssh $PI_USER@$PI_HOST "mkdir -p $PI_DIR/backend $PI_DIR/frontend"

# Copy files
echo "Copying backend files..."
scp -r ../deploy/backend/* $PI_USER@$PI_HOST:$PI_DIR/backend/

echo "Copying frontend files..."
scp -r ../deploy/frontend/* $PI_USER@$PI_HOST:$PI_DIR/frontend/

# Setup services
echo "Setting up services..."
scp ../deploy/archi-server.service $PI_USER@$PI_HOST:/tmp/
scp ../deploy/archi-nginx.conf $PI_USER@$PI_HOST:/tmp/

ssh $PI_USER@$PI_HOST "sudo mv /tmp/archi-server.service /etc/systemd/system/ && \
                       sudo systemctl daemon-reload && \
                       sudo systemctl enable archi-server && \
                       sudo systemctl restart archi-server && \
                       sudo mv /tmp/archi-nginx.conf /etc/nginx/sites-available/archi && \
                       sudo ln -sf /etc/nginx/sites-available/archi /etc/nginx/sites-enabled/ && \
                       sudo systemctl restart nginx"

echo -e "\nDeployment completed successfully!"
echo "Backend service status:"
ssh $PI_USER@$PI_HOST "sudo systemctl status archi-server --no-pager"

echo -e "\nYou can access the application at: http://$PI_HOST/"
echo "To view logs: ssh $PI_USER@$PI_HOST \"sudo journalctl -u archi-server -f\""
echo "=========================================="
