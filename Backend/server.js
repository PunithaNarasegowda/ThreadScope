
const WebSocket = require("ws");
const { spawn } = require("child_process");
const readline = require("readline");
const path = require("path");

// Create WebSocket server
const wss = new WebSocket.Server({
    port: 8080
});

console.log("ThreadScope WebSocket Server running on port 8080");

// Track engine execution
let engineStarted = false;
let childProcess = null;

// Broadcast events to all connected clients
function broadcast(data) {
    wss.clients.forEach((client) => {
        if (client.readyState === WebSocket.OPEN) {
            client.send(JSON.stringify(data));
        }
    });
}

// Start C++ engine only once
function startEngine() {

    if (engineStarted) {
        console.log("C++ Engine already started. Skipping...");
        return;
    }

    engineStarted = true;

    const executablePath = path.join(
        __dirname,
        "..",
        "cpp-engine",
        "event_producer.exe"
    );

    console.log("Starting C++ Engine...");

    childProcess = spawn(executablePath, [], {
        cwd: path.dirname(executablePath)
    });

    // Read C++ output line by line
    const rl = readline.createInterface({
        input: childProcess.stdout
    });

    // Process incoming events
    rl.on("line", (line) => {

        try {
            const event = JSON.parse(line);

            console.log("Broadcasting:", event);

            broadcast(event);

        } catch (error) {
            console.error("Invalid JSON:", line);
        }

    });

    // Capture C++ errors
    childProcess.stderr.on("data", (data) => {
        console.error("C++ ERROR:", data.toString());
    });

    // Handle startup errors
    childProcess.on("error", (error) => {
        console.error("Failed to start C++:", error.message);
        engineStarted = false;
    });

    // Detect process completion
    childProcess.on("close", (code) => {

        console.log("C++ Engine exited:", code);

        broadcast({
            type: "ENGINE_EXIT",
            code: code
        });

    });
}

// Handle WebSocket connections
wss.on("connection", (socket) => {

    console.log("React/Client connected!");

    // Start engine only on the first connection
    startEngine();

    socket.on("message", (message) => {
        console.log("Client message:", message.toString());
    });

    socket.on("close", () => {
        console.log("Client disconnected");
    });

});

