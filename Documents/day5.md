# ThreadScope – Day 6 Notes
## Instrumented Mutex and Deadlock Detection

### 1. What did we build?

We built an **Instrumented Mutex** that does more than a normal mutex.

A normal mutex protects shared resources.

An instrumented mutex additionally tracks and reports:
- Which thread requests a lock.
- Which thread successfully acquires a lock.
- Which thread releases a lock.
- Which thread is waiting for another thread.
- Whether a deadlock cycle exists.

### 2. Important events

| Event | Meaning |
|---|---|
| `LOCK_REQUESTED` | Thread wants to acquire a mutex |
| `LOCK_ACQUIRED` | Thread successfully acquired it |
| `LOCK_RELEASED` | Thread released it |
| `LOCK_TIMEOUT` | Thread could not acquire it within the time limit |

### 3. Important data structures

**Ownership map**

`ownership[mutexId] = threadId`

Tracks which thread currently owns a mutex.

Example:

`ownership[1] = 2`

Means Thread 2 owns Mutex 1.

**Waiting map**

`waiting[threadId] = mutexId`

Tracks which mutex a thread is waiting for.

Example:

`waiting[2] = 1`

Means Thread 2 is waiting for Mutex 1.

### 4. Wait-for graph

A wait-for graph represents dependencies between threads.

An edge:

`Thread A → Thread B`

means Thread A is waiting for a resource currently owned by Thread B.

Example:

- Thread 1 owns Mutex 1.
- Thread 2 owns Mutex 2.
- Thread 1 waits for Mutex 2.
- Thread 2 waits for Mutex 1.

The graph becomes:

`Thread 1 → Thread 2`

`Thread 2 → Thread 1`

This creates a cycle.

**A cycle in a single-instance-resource wait-for graph indicates deadlock.**

### 5. DFS cycle detection

We used Depth-First Search (DFS) to detect cycles in the wait-for graph.

Basic idea:
1. Start traversing from a thread.
2. Visit its dependent threads.
3. Track the current recursion path.
4. If a node appears again in the current path, a cycle exists.

Complexity:
- Time: O(V + E)
- Space: O(V)

Where V = number of threads and E = number of graph edges.

### 6. InstrumentedMutex workflow

When a thread requests a lock:

1. Log `LOCK_REQUESTED`.
2. Record the thread's waiting state.
3. Build/check the wait-for graph.
4. Attempt to acquire the real mutex.
5. If successful, update ownership and log `LOCK_ACQUIRED`.
6. If timeout occurs, remove its waiting state and log `LOCK_TIMEOUT`.

When a thread releases a lock:

1. Update ownership tracking.
2. Release the actual mutex.
3. Log `LOCK_RELEASED`.

### 7. Why did we use separate mutexes?

- `logMutex`: protects console output.
- `trackingMutex`: protects ownership and waiting metadata.
- Actual instrumented mutex: protects the shared resource.

These have different responsibilities.

### 8. Why did we use timed_mutex?

A regular mutex can wait indefinitely if a deadlock occurs.

A `timed_mutex` allows us to use:

`try_lock_for()`

This attempts to acquire a lock for a specified duration.

If acquisition fails before the timeout, the thread can exit the waiting operation rather than hanging forever.

### 9. Day 6 execution result

We tested two scenarios.

**Test 1: Controlled deadlock**

- Thread 1 acquired Mutex 1.
- Thread 2 acquired Mutex 2.
- Thread 1 requested Mutex 2.
- Thread 2 requested Mutex 1.
- The wait-for graph contained a cycle.
- DFS detected the cycle.
- Both threads eventually timed out and released their locks.

**Test 2: Normal locking**

- Thread 3 and Thread 4 requested the same mutex.
- One acquired it first.
- The other waited until it was released.
- Both completed successfully.
- No cycle was detected.

### 10. Important interview questions

**Q1. What is an instrumented mutex?**

A mutex wrapper that performs normal locking while also recording lock-related events and tracking ownership and waiting information.

**Q2. What is a wait-for graph?**

A directed graph in which an edge from one thread to another means the first thread is waiting for a resource held by the second.

**Q3. How do you detect deadlock?**

Construct a wait-for graph and use DFS to detect a directed cycle.

**Q4. Why use a separate tracking mutex?**

To prevent concurrent modifications to ownership and waiting metadata.

**Q5. Is every waiting thread deadlocked?**

No. Waiting is normal. Deadlock requires a circular dependency in the relevant wait-for graph.

**Q6. Why use `timed_mutex`?**

To attempt lock acquisition for a limited duration and avoid an indefinitely blocked demonstration.

### 11. Limitations of our current implementation

Our implementation demonstrates controlled deadlock detection. It is not yet a production-grade general-purpose detector.

- Tracking uses simplified ownership and waiting maps.
- Concurrent graph snapshots may observe intermediate states.
- One waiting entry per thread is sufficient only for our simplified demonstration.
- Manual thread IDs are used.
- A normal blocking `lock()` can still block indefinitely in an uncontrolled deadlock.

These are future hardening opportunities.

### 12. Day 6 takeaway

We connected operating-system-level concurrency concepts with a practical monitoring mechanism:

C++ threads → Instrumented mutex → Event logging → Ownership tracking → Wait-for graph → DFS cycle detection.

This becomes the foundation of ThreadScope's deadlock visualization.
