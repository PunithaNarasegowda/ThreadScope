const WebSocket = require("ws");

// Create WebSocket server
const wss = new WebSocket.Server({
    port: 8080
});

console.log("WebSocket server running on port 8080");

// Handle new client connections
wss.on("connection", (socket) => {

    console.log("Client connected!");

    // Send a message to the client
    socket.send("Welcome to ThreadScope!");

    // Receive messages from client
    socket.on("message", (message) => {
        console.log("Client says:", message.toString());
    });

    // Handle disconnection
    socket.on("close", () => {
        console.log("Client disconnected");
    });

});