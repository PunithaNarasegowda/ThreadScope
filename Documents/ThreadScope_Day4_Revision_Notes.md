# ThreadScope --- Day 4 Revision Notes

## Timestamps, JSON Serialization, and NDJSON

## 1. Day 4 objective

The goal of Day 4 was to make the C++ event collector output structured
events that another program (later, Node.js) can read.

**Day 4 deliverable:** C++ emits one structured JSON event per line.

By the end of the day, ThreadScope should: - Measure elapsed time using
`std::chrono::steady_clock`. - Convert each `ThreadEvent` into a JSON
object. - Print each JSON object on its own line (NDJSON). - Generate
and print all 18 events from six threads.

------------------------------------------------------------------------

## 2. Relative timestamps

### `system_clock` vs `steady_clock`

  -----------------------------------------------------------------------
  Clock                   What it measures        Typical use
  ----------------------- ----------------------- -----------------------
  `system_clock`          Wall-clock time,        Date/time and calendar
                          related to real-world   timestamps
                          time                    

  `steady_clock`          Monotonically           Measuring elapsed time
                          increasing time         and durations
  -----------------------------------------------------------------------

`system_clock` can be adjusted by the operating system. `steady_clock`
is designed for measuring elapsed time reliably.

### Relative timestamp

A relative timestamp measures how much time has passed since a chosen
starting point, such as when the program starts.

``` cpp
auto startTime = chrono::steady_clock::now();

long long getTimestamp() {
    return chrono::duration_cast<chrono::milliseconds>(
        chrono::steady_clock::now() - startTime
    ).count();
}
```

Explanation: 1. `steady_clock::now()` gets the current steady-clock
time. 2. Subtracting `startTime` gives the elapsed duration. 3.
`duration_cast<chrono::milliseconds>` converts that duration into
milliseconds. 4. `.count()` returns the numeric value.

Example: if 25 milliseconds have passed since the start,
`getTimestamp()` returns `25`.

**Why did the output contain many zero timestamps?**

The events were generated very quickly. Several events can happen within
the same millisecond, so they can legitimately have the same timestamp.
This does not mean the clock is broken.

For a temporary test only, adding this inside the event loop makes the
differences easier to observe:

``` cpp
this_thread::sleep_for(chrono::milliseconds(10));
```

Remove the artificial delay from the final implementation.

------------------------------------------------------------------------

## 3. JSON basics

JSON stands for **JavaScript Object Notation**. It is a structured text
format used to exchange data between programs.

Example event:

``` json
{
  "threadId": 1,
  "eventType": "START",
  "timestamp": 25,
  "resourceId": -1
}
```

-   JSON keys are strings.
-   Values can be strings, numbers, booleans, arrays, objects, or
    `null`.
-   JSON is not the same thing as a C++ `struct`; it is a text
    representation of data.

### Serialization and deserialization

-   **Serialization:** converting an in-memory object or data structure
    into a format such as JSON.
-   **Deserialization:** reading JSON and converting it back into an
    in-memory representation.

In ThreadScope, we serialize each `ThreadEvent` so Node.js can read it
later.

------------------------------------------------------------------------

## 4. The `nlohmann/json` library

We use the `nlohmann/json` single-header library to create JSON objects
from C++.

### Project folder structure

If the header is stored inside `cpp-engine/include/nlohmann/`, the
structure should look like this:

``` text
cpp-engine/
├── include/
│   └── nlohmann/
│       └── json.hpp
├── Threaevent.cpp
├── main.cpp
├── counter_mutex.cpp
├── counter_race.cpp
└── deadlock_demo.cpp
```

### Include the library

With the folder structure above, use:

``` cpp
#include "nlohmann/json.hpp"
using json = nlohmann::json;
```

Compile from the `cpp-engine` directory with the include directory
specified:

``` bash
g++ -std=c++17 -Iinclude Threaevent.cpp -o ThreadScope -pthread
```

Run in PowerShell:

``` powershell
.\ThreadScope
```

The `-Iinclude` option tells the compiler to search the `include`
directory for headers. The `-pthread` option enables the C++ threading
support needed by this program.

------------------------------------------------------------------------

## 5. The `ThreadEvent` structure

Our event structure groups the information associated with one event:

``` cpp
struct ThreadEvent {
    int threadId;
    string eventType;
    long long timestamp;
    int resourceId;
};
```

  -----------------------------------------------------------------------
  Field                               Meaning
  ----------------------------------- -----------------------------------
  `threadId`                          Identifies the thread that
                                      generated the event

  `eventType`                         Describes what happened, such as
                                      `START`, `WORKING`, or `FINISH`

  `timestamp`                         Relative time at which the event
                                      was recorded

  `resourceId`                        Identifies an associated resource;
                                      `-1` means no resource is involved
  -----------------------------------------------------------------------

For the current START, WORKING, and FINISH events, we use
`resourceId = -1`. Later, lock-related events can use actual resource
IDs.

------------------------------------------------------------------------

## 6. Serialize `ThreadEvent` using `toJson()`

``` cpp
json toJson(const ThreadEvent& event) {
    return {
        {"threadId", event.threadId},
        {"eventType", event.eventType},
        {"timestamp", event.timestamp},
        {"resourceId", event.resourceId}
    };
}
```

How to read one entry:

``` cpp
{"threadId", event.threadId}
```

-   `"threadId"` is the JSON key.
-   `event.threadId` is the value taken from the C++ struct.

The function accepts a `ThreadEvent` by constant reference and returns a
JSON object.

### What does `.dump()` do?

``` cpp
cout << toJson(event).dump() << '\n';
```

-   `toJson(event)` creates a JSON object.
-   `.dump()` converts that JSON object into a JSON-formatted string.
-   `cout` prints the string.
-   `'\n'` ends the line.

We print the serialized JSON string, rather than trying to print the C++
struct directly.

------------------------------------------------------------------------

## 7. NDJSON

**NDJSON** means **Newline Delimited JSON**.

It means each line contains one independent JSON object.

Example:

``` json
{"threadId":1,"eventType":"START","timestamp":5,"resourceId":-1}
{"threadId":2,"eventType":"START","timestamp":7,"resourceId":-1}
{"threadId":1,"eventType":"WORKING","timestamp":10,"resourceId":-1}
{"threadId":2,"eventType":"FINISH","timestamp":15,"resourceId":-1}
```

Important characteristics: - Each line is a complete JSON object. - Each
line represents one event. - There is no outer array around all
events. - A newline separates one event from the next.

### Why ThreadScope uses NDJSON

Later, Node.js will launch the C++ program and read its standard output.
It can split the output into lines and parse each complete JSON event.

A stream reader must handle two cases: - A received chunk may contain
only part of a JSON line. - A received chunk may contain several
complete JSON lines.

We will implement that buffering and line splitting when we reach the
Node.js integration day.

------------------------------------------------------------------------

## 8. Thread-safe event collection and output

The shared vector is protected by a mutex while worker threads insert
events:

``` cpp
vector<ThreadEvent> events;
mutex eventsMutex;

void producer(ThreadEvent event) {
    lock_guard<mutex> lock(eventsMutex);
    events.push_back(event);
}
```

Each worker generates three events:

``` cpp
void generateEvents(int threadId) {
    for (const char* eventType : {"START", "WORKING", "FINISH"}) {
        ThreadEvent event;
        event.threadId = threadId;
        event.eventType = eventType;
        event.timestamp = getTimestamp();
        event.resourceId = -1;

        producer(event);
    }
}
```

After all worker threads have been joined, the main thread prints the
collected events:

``` cpp
for (thread& worker : threads) {
    worker.join();
}

for (const ThreadEvent& event : events) {
    cout << toJson(event).dump() << '\n';
}
```

**Why is another mutex not needed while printing?**

After all worker threads have been joined, they have finished inserting
events. No worker is still modifying the vector, so the main thread can
safely read it.

**Does `join()` guarantee a particular event order?**

No. It waits for a thread to finish. It does not guarantee that
different threads execute or insert events in thread-ID order.

Also, timestamps are sampled before insertion into the vector. Under
concurrency, vector insertion order is not guaranteed to be exactly the
same as timestamp order.

------------------------------------------------------------------------

## 9. Complete Day 4 implementation

This is the version matching the implementation tested during Day 4:

``` cpp
#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>
#include <vector>
#include <string>
#include "nlohmann/json.hpp"

using json = nlohmann::json;
using namespace std;

struct ThreadEvent {
    int threadId;
    string eventType;
    long long timestamp;
    int resourceId;
};

json toJson(const ThreadEvent& event) {
    return {
        {"threadId", event.threadId},
        {"eventType", event.eventType},
        {"timestamp", event.timestamp},
        {"resourceId", event.resourceId}
    };
}

vector<ThreadEvent> events;
mutex eventsMutex;

void producer(ThreadEvent event) {
    lock_guard<mutex> lock(eventsMutex);
    events.push_back(event);
}

auto startTime = chrono::steady_clock::now();

long long getTimestamp() {
    return chrono::duration_cast<chrono::milliseconds>(
        chrono::steady_clock::now() - startTime
    ).count();
}

void generateEvents(int threadId) {
    for (const char* eventType : {"START", "WORKING", "FINISH"}) {
        ThreadEvent event;
        event.threadId = threadId;
        event.eventType = eventType;
        event.timestamp = getTimestamp();
        event.resourceId = -1;

        producer(event);
    }
}

int main() {
    vector<thread> threads;

    for (int threadId = 1; threadId <= 6; ++threadId) {
        threads.emplace_back(generateEvents, threadId);
    }

    for (thread& worker : threads) {
        worker.join();
    }

    for (const ThreadEvent& event : events) {
        cout << toJson(event).dump() << '\n';
    }

    return 0;
}
```

Compile:

``` bash
g++ -std=c++17 -Iinclude Threaevent.cpp -o ThreadScope -pthread
```

Run:

``` powershell
.\ThreadScope
```

------------------------------------------------------------------------

## 10. Expected output and verification

Each thread generates: - One `START` event. - One `WORKING` event. - One
`FINISH` event.

Therefore:

**6 threads × 3 events = 18 JSON events.**

The tested program successfully printed 18 JSON lines. The output
included relative timestamps such as `0` and `1` milliseconds. Many
events shared the same timestamp because they occurred within the same
millisecond.

The order of events and the exact timestamps can vary between runs.

------------------------------------------------------------------------

## 11. Common mistakes to remember

  -----------------------------------------------------------------------
  Mistake                             Correction
  ----------------------------------- -----------------------------------
  Printing the struct directly        Convert it with `toJson()` and
                                      print `.dump()`

  Printing ordinary text instead of   Print one serialized JSON object
  JSON                                per event

  Using `system_clock` for elapsed    Use `steady_clock`
  time                                

  Expecting every event to have a     Multiple events can occur within
  unique millisecond timestamp        one millisecond

  Forgetting the include directory    Add `-Iinclude` when using the
  during compilation                  shown folder structure

  Expecting threads to print in       Thread scheduling determines
  numerical order                     execution order

  Wrapping all events in an array     Keep one JSON object per line for
                                      NDJSON
  -----------------------------------------------------------------------

------------------------------------------------------------------------

## 12. Interview / revision questions

1.  What is the difference between `system_clock` and `steady_clock`?
2.  Why is `steady_clock` suitable for measuring elapsed time?
3.  What is a relative timestamp?
4.  What is JSON?
5.  What is serialization? What is deserialization?
6.  What is the purpose of the `nlohmann/json` library?
7.  What does `toJson()` do in ThreadScope?
8.  What does `.dump()` return?
9.  What is NDJSON?
10. Why are we using one JSON object per line?
11. Why can multiple events have the same timestamp?
12. Why can the event insertion order differ from timestamp order?
13. Why can the main thread print the event vector after joining all
    workers without locking the vector again?

### Quick answers

1.  `system_clock` represents wall-clock time; `steady_clock` measures
    monotonic elapsed time.
2.  It is not affected by wall-clock adjustments in the way
    `system_clock` can be.
3.  Time elapsed since a chosen starting point.
4.  A structured text format for representing and exchanging data.
5.  Serialization converts data into a format such as JSON;
    deserialization converts it back.
6.  It provides C++ types and functions for creating and working with
    JSON.
7.  It converts a `ThreadEvent` into a JSON object.
8.  A JSON-formatted string.
9.  Newline Delimited JSON: one JSON object per line.
10. It makes individual events easy for Node.js to read and parse from a
    stream.
11. They can occur within the same millisecond.
12. The timestamp is sampled before insertion, and thread scheduling can
    change insertion order.
13. All worker threads have finished, so no worker is concurrently
    modifying the vector.

------------------------------------------------------------------------

## 13. Day 4 completion checklist

-   [x] Understood relative timestamps and `steady_clock`.
-   [x] Understood JSON and serialization.
-   [x] Installed and tested `nlohmann/json`.
-   [x] Serialized `ThreadEvent` using `toJson()`.
-   [x] Printed JSON using `.dump()`.
-   [x] Understood NDJSON.
-   [x] Generated and verified 18 JSON event lines.

**Day 4 complete.**

------------------------------------------------------------------------

## Next: Day 5 --- Deadlocks and cycle detection

The next day covers: 1. What is a deadlock? 2. The four necessary
conditions for deadlock. 3. Deadlock vs starvation vs livelock. 4.
Resource allocation graphs and wait-for graphs. 5. DFS-based cycle
detection.

**Day 5 deliverable:** A controlled deadlock demonstration and a
standalone graph cycle detector.
