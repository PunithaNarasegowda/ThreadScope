const { spawn } = require("child_process");
const readline = require("readline");
const path = require("path");

console.log("Starting ThreadScope C++ Engine...");

// Find the C++ executable
const executablePath = path.join(
    __dirname,
    "..",
    "cpp-engine",
    "event_producer.exe"
);

// Start the C++ process
const child = spawn(executablePath, [], {
    cwd: path.dirname(executablePath)
});

// Read C++ output line by line
const rl = readline.createInterface({
    input: child.stdout
});

// Process every JSON event
rl.on("line", (line) => {

    try {
        const event = JSON.parse(line);

        console.log("Received C++ Event:");
        console.log("Thread ID:", event.threadId);
        console.log("Event Type:", event.eventType);
        console.log("Timestamp:", event.timestamp);

        console.log("--------------------");

    } catch (error) {
        console.error("Invalid JSON event:", line);
    }

});

// Capture C++ errors
child.stderr.on("data", (data) => {
    console.error("C++ ERROR:", data.toString());
});

// Handle process startup failure
child.on("error", (error) => {
    console.error("Failed to start C++ engine:", error.message);
});

// Detect process completion
child.on("close", (code) => {
    console.log("C++ Engine exited with code:", code);
});