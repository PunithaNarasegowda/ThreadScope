
const WebSocket = require("ws");
const { spawn } = require("child_process");
const readline = require("readline");
const path = require("path");

const wss = new WebSocket.Server({
    port: 8080
});

console.log("ThreadScope WebSocket Server running on port 8080");

// Store all generated events
const eventHistory = [];

// Prevent multiple C++ engine executions
let engineStarted = false;

// Broadcast and save events

function broadcast(data) {

    eventHistory.push(data);

    console.log("Sending event:", data);
    console.log("Connected clients:", wss.clients.size);

    wss.clients.forEach((client) => {

        console.log("Client readyState:", client.readyState);

        if (client.readyState === WebSocket.OPEN) {

            client.send(JSON.stringify(data), (error) => {

                if (error) {
                    console.error("Send failed:", error);
                } else {
                    console.log("Event delivered successfully!");
                }

            });

        }

    });

}

// Start C++ engine
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

    const child = spawn(executablePath, [], {
        cwd: path.dirname(executablePath)
    });

    const rl = readline.createInterface({
        input: child.stdout
    });

    // Receive events from C++
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
    child.stderr.on("data", (data) => {
        console.error("C++ ERROR:", data.toString());
    });

    // Handle startup errors
    child.on("error", (error) => {
        console.error("Failed to start C++:", error.message);

        broadcast({
            type: "ENGINE_EXIT",
            code: -1
        });
    });

    // Detect completion
    child.on("close", (code) => {

        console.log("C++ Engine exited:", code);

        broadcast({
            type: "ENGINE_EXIT",
            code: code
        });

    });

}

// Handle React connections
wss.on("connection", (socket) => {

    console.log("React/Client connected!");

    // Replay previously generated events
    console.log("Sending event history:", eventHistory.length);

    eventHistory.forEach((event) => {
        if (socket.readyState === WebSocket.OPEN) {
            socket.send(JSON.stringify(event));
        }
    });

    // Start engine only if it hasn't started
    startEngine();

    socket.on("message", (message) => {
        console.log("Client message:", message.toString());
    });

    socket.on("close", () => {
        console.log("Client disconnected");
    });

});
