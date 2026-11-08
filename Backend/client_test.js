const WebSocket = require("ws");

const socket = new WebSocket("ws://localhost:8080");

socket.on("open", () => {
    console.log("Connected to ThreadScope!");

});

socket.on("message", (data) => {

    const event = JSON.parse(data.toString());

    if (event.type === "ENGINE_EXIT") {
        console.log("C++ Engine finished!");
        console.log("Exit code:", event.code);
        socket.close();
        return;
    }

    console.log("Received Thread Event:");
    console.log("Thread ID:", event.threadId);
    console.log("Event Type:", event.eventType);
    console.log("Timestamp:", event.timestamp);
    console.log("--------------------");

});

socket.on("close", () => {
    console.log("WebSocket connection closed.");
});

socket.on("error", (error) => {
    console.error("WebSocket error:", error.message);
});