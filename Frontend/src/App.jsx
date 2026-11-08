
import { useEffect, useState } from "react";
import "./index.css";

function App() {
  const [threads, setThreads] = useState({
    1: { status: "WAITING", events: [] },
    2: { status: "WAITING", events: [] },
    3: { status: "WAITING", events: [] },
  });

  const [events, setEvents] = useState([]);
  const [connection, setConnection] = useState("Connecting...");
  const [engineStatus, setEngineStatus] = useState("Waiting");

  useEffect(() => {
    const socket = new WebSocket("ws://localhost:8080");

    socket.onopen = () => {
      setConnection("Connected");
      console.log("WebSocket connected!");
    };

    socket.onmessage = (message) => {
      console.log("RAW WEBSOCKET MESSAGE:", message.data);
      try {
        const event = JSON.parse(message.data);

        console.log("Received event:", event);

        // Handle engine completion separately
        if (event.type === "ENGINE_EXIT") {
          setEngineStatus(
            event.code === 0 ? "Completed" : "Failed"
          );
          return;
        }

        // Ignore invalid or unexpected thread IDs
        if (![1, 2, 3].includes(event.threadId)) {
          return;
        }

        setEngineStatus("Running");

        // Update the global event timeline
        setEvents((previousEvents) => [
          ...previousEvents,
          event,
        ]);

        // Update the individual thread
        setThreads((previousThreads) => ({
          ...previousThreads,
          [event.threadId]: {
            status: event.eventType,
            events: [
              ...previousThreads[event.threadId].events,
              event,
            ],
          },
        }));
      } catch (error) {
        console.error("Error parsing WebSocket message:", error);
      }
    };

    socket.onerror = () => {
      setConnection("Connection Error");
    };

    socket.onclose = () => {
      setConnection("Disconnected");
    };

    return () => {
      socket.close();
    };
  }, []);

  // Calculate dashboard statistics
  const totalThreads = Object.keys(threads).length;

  const completedThreads = Object.values(threads).filter(
    (thread) => thread.status === "FINISH"
  ).length;

  const totalEvents = events.length;

  // Status colors
  const statusColors = {
    WAITING: "#64748b",
    START: "#3b82f6",
    WORKING: "#eab308",
    FINISH: "#22c55e",
  };
useEffect(() => {
  console.log("THREAD STATE:", threads);
  console.log("TOTAL EVENTS:", events.length);
  console.log("ENGINE STATUS:", engineStatus);
}, [threads, events, engineStatus]);
  return (
    <div className="dashboard">

      {/* Header */}
      <header className="header">
        <div>
          <h1>ThreadScope</h1>
          <p>Real-Time C++ Multithreading Visualizer</p>
        </div>

        <div className={`connection ${connection === "Connected" ? "online" : "offline"}`}>
          <span className="connection-dot"></span>
          {connection}
        </div>
      </header>

      {/* Statistics */}
      <section className="stats-grid">

        <div className="stat-card">
          <p>Total Threads</p>
          <h2>{totalThreads}</h2>
        </div>

        <div className="stat-card">
          <p>Completed Threads</p>
          <h2>{completedThreads}/{totalThreads}</h2>
        </div>

        <div className="stat-card">
          <p>Total Events</p>
          <h2>{totalEvents}</h2>
        </div>

        <div className="stat-card">
          <p>Engine Status</p>
          <h2 className="engine-status">{engineStatus}</h2>
        </div>

      </section>

      {/* Thread Monitoring */}
      <section className="section">
        <h2>Thread Monitoring</h2>

        <div className="thread-grid">

          {Object.entries(threads).map(([threadId, thread]) => (
            <div className="thread-card" key={threadId}>

              <div className="thread-card-header">
                <h3>Thread {threadId}</h3>

                <span
                  className="status-badge"
                  style={{
                    backgroundColor:
                      statusColors[thread.status] || "#64748b",
                  }}
                >
                  {thread.status}
                </span>
              </div>

              <div className="thread-info">
                <p>
                  Events received:
                  <strong>{thread.events.length}</strong>
                </p>

                <p>
                  Latest event:
                  <strong>
                    {thread.events.length > 0
                      ? thread.events[thread.events.length - 1].eventType
                      : "None"}
                  </strong>
                </p>
              </div>

              <div className="thread-progress">
                <div
                  className="thread-progress-bar"
                  style={{
                    width:
                      thread.status === "FINISH"
                        ? "100%"
                        : thread.status === "WORKING"
                        ? "66%"
                        : thread.status === "START"
                        ? "33%"
                        : "0%",
                    backgroundColor:
                      statusColors[thread.status],
                  }}
                ></div>
              </div>

            </div>
          ))}

        </div>
      </section>

      {/* Event Timeline */}
      <section className="section">
        <div className="section-heading">
          <h2>Live Event Timeline</h2>
          <span>{totalEvents} events</span>
        </div>

        <div className="timeline">

          {events.length === 0 ? (
            <p className="empty-message">
              Waiting for thread events...
            </p>
          ) : (
            [...events].reverse().map((event, index) => (
              <div className="timeline-item" key={`${event.threadId}-${index}`}>

                <div
                  className="timeline-indicator"
                  style={{
                    backgroundColor:
                      statusColors[event.eventType] || "#64748b",
                  }}
                ></div>

                <div className="timeline-content">
                  <h4>
                    Thread {event.threadId}
                    <span
                      className="timeline-status"
                      style={{
                        color: statusColors[event.eventType],
                      }}
                    >
                      {event.eventType}
                    </span>
                  </h4>

                  <p>
                    Timestamp: {event.timestamp} ms
                  </p>
                </div>

              </div>
            ))
          )}

        </div>
      </section>

      {/* Footer */}
      <footer>
        <p>ThreadScope | C++ Multithreading Visualizer</p>
      </footer>

    </div>
  );
}

export default App;
