import React, { useState, useEffect, useRef } from 'react';
import * as signalR from '@microsoft/signalr';
import './App.css';

function App() {
    const [cameraFeed, setCameraFeed] = useState('');
    const [connectionStatus, setConnectionStatus] = useState('disconnected');
    const [lastFrameTime, setLastFrameTime] = useState(null);
    const [frameCount, setFrameCount] = useState(0);
    const [fps, setFps] = useState(0);
    const connectionRef = useRef(null);
    const frameCountRef = useRef(0);
    const lastTimeRef = useRef(Date.now());

    useEffect(() => {
        // Function to establish connection to SignalR hub
        const connectToHub = () => {
            try {
                // Get server URL from environment or use default
                const serverUrl = process.env.REACT_APP_SERVER_URL || 'http://localhost:5000';

                setConnectionStatus('connecting');

                const connection = new signalR.HubConnectionBuilder()
                    .withUrl(`${serverUrl}/cameraHub`)
                    .withAutomaticReconnect([0, 1000, 5000, 10000]) // Retry policy
                    .configureLogging(signalR.LogLevel.Warning)
                    .build();

                // Handle connection events
                connection.onclose(() => {
                    console.log('Connection closed');
                    setConnectionStatus('disconnected');
                });

                connection.onreconnecting(() => {
                    console.log('Reconnecting...');
                    setConnectionStatus('connecting');
                });

                connection.onreconnected(() => {
                    console.log('Reconnected!');
                    setConnectionStatus('connected');
                });

                // Set up handler for receiving frames
                connection.on('ReceiveFrame', (base64Image) => {
                    setCameraFeed(`data:image/jpeg;base64,${base64Image}`);
                    setLastFrameTime(new Date());

                    // Track frame rate
                    frameCountRef.current++;
                    setFrameCount(prev => prev + 1);

                    const now = Date.now();
                    const elapsed = now - lastTimeRef.current;
                    if (elapsed >= 1000) { // Update FPS every second
                        setFps(Math.round((frameCountRef.current * 1000) / elapsed));
                        frameCountRef.current = 0;
                        lastTimeRef.current = now;
                    }
                });

                // Start the connection
                connection.start()
                    .then(() => {
                        console.log('Connected to camera hub!');
                        setConnectionStatus('connected');
                    })
                    .catch(err => {
                        console.error('Error connecting to hub:', err);
                        setConnectionStatus('disconnected');

                        // Try to reconnect after a delay
                        setTimeout(connectToHub, 5000);
                    });

                // Store connection for cleanup
                connectionRef.current = connection;
            } catch (error) {
                console.error('Error setting up connection:', error);
                setConnectionStatus('disconnected');
            }
        };

        // Initialize connection
        connectToHub();

        // Cleanup on unmount
        return () => {
            if (connectionRef.current) {
                connectionRef.current.stop();
            }
        };
    }, []);

    // Format the last frame time
    const formattedTime = lastFrameTime ? lastFrameTime.toLocaleTimeString() : 'Never';

    return (
        <div className="app-container">
            <header className="app-header">
                <h1>ArchI ESP32-CAM Stream</h1>
                <div className="status-bar">
                    <div className={`connection-status ${connectionStatus}`}>
                        {connectionStatus === 'connected' ? 'Connected' :
                            connectionStatus === 'connecting' ? 'Connecting...' : 'Disconnected'}
                    </div>
                </div>
            </header>

            <main className="content">
                <div className="camera-container">
                    {cameraFeed ? (
                        <>
                            <img
                                src={cameraFeed}
                                alt="ESP32-CAM Stream"
                                className="camera-feed"
                            />
                            <div className={`status-indicator status-${connectionStatus === 'connected' ? 'online' : 'offline'}`}></div>
                        </>
                    ) : (
                        <div className="camera-offline">
                            <h3>Camera Feed Offline</h3>
                            <p>Waiting for connection to the ESP32-CAM...</p>
                        </div>
                    )}
                </div>

                <div className="stream-stats">
                    <div className="stat-item">
                        <span className="stat-label">Status:</span>
                        <span className={`stat-value ${connectionStatus}`}>
                            {connectionStatus === 'connected' ? 'Online' :
                                connectionStatus === 'connecting' ? 'Connecting...' : 'Offline'}
                        </span>
                    </div>
                    <div className="stat-item">
                        <span className="stat-label">Last Frame:</span>
                        <span className="stat-value">{formattedTime}</span>
                    </div>
                    <div className="stat-item">
                        <span className="stat-label">Frame Rate:</span>
                        <span className="stat-value">{fps} FPS</span>
                    </div>
                    <div className="stat-item">
                        <span className="stat-label">Total Frames:</span>
                        <span className="stat-value">{frameCount}</span>
                    </div>
                </div>
            </main>

            <footer className="app-footer">
                <p>ArchI ESP32-CAM Streaming Project &copy; {new Date().getFullYear()}</p>
            </footer>
        </div>
    );
}

export default App;
