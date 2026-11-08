# ThreadScope – Day 7 Revision Notes
## C++ and Node.js Integration

### 1. What did we build?

We connected our C++ multithreading engine with a Node.js backend.

Previously:
- C++ executed independently.
- Node.js executed independently.

Now:
- Node.js launches the C++ executable.
- C++ generates structured events.
- Node.js receives those events.
- Node.js parses the JSON and converts it into JavaScript objects.

Architecture:

C++ Engine → Node.js Backend → WebSocket → React Dashboard

### 2. What is Node.js?

Node.js is a JavaScript runtime that allows JavaScript to execute outside the browser.

We use it for our ThreadScope backend.

### 3. What is an HTTP server?

An HTTP server receives client requests and sends responses.

We created two routes:

| Route | Purpose |
|---|---|
| `/` | Backend status |
| `/health` | Health check |

Important methods:
- `http.createServer()` – creates the server.
- `req` – incoming request.
- `res` – outgoing response.
- `res.writeHead()` – sets HTTP status and headers.
- `res.end()` – sends the response.
- `server.listen(3000)` – starts the server.

### 4. What is a child process?

A child process is a separate program started by another program.

In our project:
- Parent process: Node.js.
- Child process: C++ executable.

### 5. What is spawn()?

`spawn()` from Node.js's `child_process` module starts another program.

Example:

`const { spawn } = require("child_process");`

`const child = spawn("program.exe");`

We used it to launch `event_producer.exe`.

### 6. Standard streams

| Stream | Meaning |
|---|---|
| stdin | Standard input |
| stdout | Standard output |
| stderr | Standard error |

In ThreadScope:
- `child.stdout` receives normal C++ output.
- `child.stderr` receives diagnostic/error output.

### 7. Child process events

**stdout data**

`child.stdout.on("data", callback)`

Receives output chunks.

**stderr data**

`child.stderr.on("data", callback)`

Receives diagnostic output.

**error**

`child.on("error", callback)`

Handles failures when starting the process.

Example: executable not found (`ENOENT`).

**close**

`child.on("close", callback)`

Runs after the child process has terminated and its streams have closed.

Exit code `0` indicates successful completion.

### 8. What is NDJSON?

NDJSON = Newline Delimited JSON.

Each line contains one independent JSON object.

Example:

`{"threadId":1,"eventType":"START","timestamp":0}`

`{"threadId":1,"eventType":"FINISH","timestamp":1000}`

Why use it?
- Easy to stream.
- Easy to parse line by line.
- Suitable for sending real-time events.

### 9. C++ event producer

We created `event_producer.cpp`.

It generated:
- START
- WORKING
- FINISH

Each event contained:
- `threadId`
- `eventType`
- `timestamp`

Important C++ concepts:
- `steady_clock` – measures elapsed time.
- `duration_cast<milliseconds>` – converts elapsed duration into milliseconds.
- `mutex` – prevents simultaneous output writes.
- `lock_guard` – automatically locks and unlocks the mutex.
- `thread.join()` – waits for worker completion.
- `endl` – ends a line and flushes the stream.

### 10. Why use readline?

Node.js stdout receives data in chunks, not guaranteed complete JSON lines.

The `readline` module separates incoming data into complete lines.

Example:

`const readline = require("readline");`

`const rl = readline.createInterface({ input: child.stdout });`

`rl.on("line", (line) => { ... });`

### 11. What is JSON.parse()?

It converts JSON text into a JavaScript object.

Example:

`const event = JSON.parse(line);`

We can then access:

- `event.threadId`
- `event.eventType`
- `event.timestamp`

### 12. Final architecture

1. C++ generates NDJSON events.
2. Node.js starts the executable using `spawn()`.
3. stdout transfers events.
4. readline separates complete lines.
5. JSON.parse() creates JavaScript objects.
6. Node.js can forward these objects through WebSockets.

### 13. Interview questions

**Q1. What is Node.js?**

A JavaScript runtime that executes JavaScript outside the browser.

**Q2. What is spawn()?**

A function from Node.js's child_process module that starts another process.

**Q3. What is stdout?**

The standard output stream used to transfer normal program output.

**Q4. What is stderr?**

The standard error stream used for diagnostic and error messages.

**Q5. What is NDJSON?**

A format where each newline-delimited line is an independent JSON object.

**Q6. Why use readline?**

To process complete lines from a stream rather than assuming each incoming chunk is one complete event.

**Q7. What does JSON.parse() do?**

Converts JSON-formatted text into a JavaScript object.

**Q8. What does ENOENT mean?**

The requested file or executable could not be found.

**Q9. Why use spawn() instead of manually running C++?**

It allows the backend to launch and monitor the C++ engine programmatically.

**Q10. What does exit code 0 mean?**

The child process completed successfully.

### 14. Day 7 takeaway

We successfully established communication between C++ and Node.js.

C++ multithreading → NDJSON → spawn() → stdout → readline → JSON.parse() → JavaScript event objects.

The parsed events are now ready to be broadcast to our frontend using WebSockets.