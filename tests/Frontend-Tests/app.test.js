import React from 'react';
import { render, screen, waitFor, act } from '@testing-library/react';
import '@testing-library/jest-dom';
import App from '../../WebServer/Frontend/src/App';

// Mock the SignalR connection
jest.mock('@microsoft/signalr', () => {
    const actual = jest.requireActual('@microsoft/signalr');
    const mockConnection = {
        start: jest.fn().mockResolvedValue(),
        stop: jest.fn().mockResolvedValue(),
        on: jest.fn(),
        onclose: jest.fn(),
        onreconnecting: jest.fn(),
        onreconnected: jest.fn(),
        invoke: jest.fn(),
    };

    return {
        ...actual,
        HubConnectionBuilder: jest.fn().mockImplementation(() => {
            return {
                withUrl: jest.fn().mockReturnThis(),
                withAutomaticReconnect: jest.fn().mockReturnThis(),
                configureLogging: jest.fn().mockReturnThis(),
                build: jest.fn().mockImplementation(() => mockConnection),
            };
        }),
        mockConnection,
    };
});

describe('App Component', () => {
    test('renders the header correctly', () => {
        render(<App />);
        const headerElement = screen.getByText(/ArchI ESP32-CAM Stream/i);
        expect(headerElement).toBeInTheDocument();
    });

    test('shows camera offline message when no feed is available', () => {
        render(<App />);
        const offlineElement = screen.getByText(/Camera Feed Offline/i);
        expect(offlineElement).toBeInTheDocument();
    });

    test('connection status changes to connected after successful connection', async () => {
        render(<App />);

        // Initially status should be connecting or disconnected
        const initialStatus = screen.getByText(/connecting|disconnected/i);
        expect(initialStatus).toBeInTheDocument();

        // Wait for the connection to be established (mock)
        await waitFor(() => {
            // Simulate a successful connection by triggering the onreconnected callback
            const { mockConnection } = require('@microsoft/signalr');
            const onreconnectedCallback = mockConnection.onreconnected.mock.calls[0][0];
            act(() => {
                onreconnectedCallback();
            });
        });

        // Check if status changed to Connected
        const connectedStatus = screen.getByText(/online/i);
        expect(connectedStatus).toBeInTheDocument();
    });

    test('displays an image when receiving a frame', async () => {
        render(<App />);

        // Simulate receiving a frame
        const { mockConnection } = require('@microsoft/signalr');
        const onCallback = mockConnection.on.mock.calls.find(call => call[0] === 'ReceiveFrame');
        const receiveFrameCallback = onCallback ? onCallback[1] : null;

        // Trigger the callback with a base64 image
        const testBase64Image = 'dGVzdEltYWdlRGF0YQ=='; // 'testImageData' in base64

        await act(async () => {
            receiveFrameCallback(testBase64Image);
        });

        // Check if an image is rendered
        const imageElement = screen.getByAltText(/ESP32-CAM Stream/i);
        expect(imageElement).toBeInTheDocument();
        expect(imageElement).toHaveAttribute('src', `data:image/jpeg;base64,${testBase64Image}`);
    });
});
