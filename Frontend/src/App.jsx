
import { useEffect, useState } from "react";

function App() {
  const [events, setEvents] = useState([]);
  const [connection, setConnection] = useState("Connecting...");

  useEffect(() => {
    const socket = new WebSocket("ws://localhost:8080");

    socket.onopen = () => {
      setConnection("Connected");
      console.log("WebSocket connection established!");
    };

    socket.onmessage = (message) => {
      const event = JSON.parse(message.data);

      console.log("Received event:", event);

      setEvents((previousEvents) => [
        ...previousEvents,
        event
      ]);
    };

    socket.onerror = () => {
      setConnection("Connection Error");
    };

    socket.onclose = () => {
      setConnection("Disconnected");
    };

    return () => socket.close();
  }, []);

  return (
    <div style={{
      padding: "30px",
      fontFamily: "Arial",
      color: "white",
      minHeight: "100vh",
      background: "#16171d"
    }}>

      <h1>ThreadScope</h1>

      <h3>Real-Time C++ Multithreading Visualizer</h3>

      <p>WebSocket Status: {connection}</p>

      <h2>Live Thread Events</h2>

     

      {events.map((event, index) => (
        <div
          key={index}
          style={{
            padding: "12px",
            margin: "8px 0",
            background: "#252630",
            borderRadius: "8px"
          }}
        >
          {event.eventType ? (
            <>
              Thread {event.threadId}
              {" - "}
              {event.eventType}
              {" - "}
              {event.timestamp} ms
            </>
          ) : (
            <>
              Engine Status: {event.type}
              {" - Exit Code: "}
              {event.code}
            </>
          )}
        </div>
      ))}

      

    </div>
  );
}

export default App;
