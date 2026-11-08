# ThreadScope — Day 5 Revision Notes
## Deadlocks, Wait-for Graphs, and DFS Cycle Detection

## 1. Day 5 objective

Day 5 combined operating-systems deadlock concepts with a first C++ cycle detector.

**Day 5 deliverables:**
- Understand deadlocks and the four necessary conditions.
- Distinguish deadlock, starvation, and livelock.
- Represent waiting relationships as a directed graph.
- Detect directed cycles using DFS.
- Run a controlled two-mutex deadlock scenario with timeouts.

---

## 2. What is a deadlock?

A deadlock occurs when two or more threads are blocked because each is waiting for a resource held by another thread in the group.

Example:
- Thread 1 holds Mutex 1 and requests Mutex 2.
- Thread 2 holds Mutex 2 and requests Mutex 1.

If both acquire their first mutex before requesting the second, neither can proceed until the other releases its mutex.

```text
Thread 1 holds Mutex 1; waits for Mutex 2
Thread 2 holds Mutex 2; waits for Mutex 1
```

---

## 3. The four necessary conditions for deadlock

In the classical model, all four conditions must hold simultaneously for deadlock to occur.

| Condition | Meaning |
|---|---|
| Mutual exclusion | A resource can be held by only one thread at a time. |
| Hold and wait | A thread holds one resource while waiting for another. |
| No preemption | A resource cannot be forcibly taken from its owner; it must be released by the owner. |
| Circular wait | A circular chain exists in which each thread waits for a resource held by the next thread. |

For the two-thread example:
- Mutual exclusion: only one thread owns each mutex.
- Hold and wait: each thread holds one mutex and requests another.
- No preemption: neither thread can forcibly take the other's mutex.
- Circular wait: Thread 1 waits for Thread 2, and Thread 2 waits for Thread 1.

A common prevention approach is to break at least one of these conditions. For example, acquiring multiple locks in a consistent global order can prevent circular wait.

---

## 4. Deadlock vs starvation vs livelock

| Concept | What happens? | Do other threads make progress? |
|---|---|---|
| Deadlock | Threads are blocked waiting on one another in a cycle. | The deadlocked group does not progress. |
| Starvation | A thread is repeatedly denied access to a resource or execution opportunity. | Other threads may continue progressing. |
| Livelock | Threads remain active and react to each other, but make no useful progress. | They execute, but the intended work does not progress. |

Examples:
- **Deadlock:** T1 waits for a mutex held by T2, while T2 waits for a mutex held by T1.
- **Starvation:** T1 repeatedly loses access to a resource while other threads keep acquiring it.
- **Livelock:** Two people repeatedly move in the same direction to avoid each other in a narrow corridor, so neither passes.

---

## 5. Resource Allocation Graph (RAG)

A Resource Allocation Graph represents threads, resources, and their relationships.

It has two kinds of nodes:
- Thread nodes: T1, T2, etc.
- Resource nodes: R1, R2, etc.

It has two kinds of directed edges:
- **Request edge:** Thread → Resource. The thread is requesting the resource.
- **Assignment edge:** Resource → Thread. The resource is allocated to the thread.

Example:
- T1 holds R1 and requests R2.
- T2 holds R2 and requests R1.

The graph contains this directed cycle:

```text
T1 → R2 → T2 → R1 → T1
```

For resources with a single instance each, a cycle in the resource-allocation graph indicates deadlock. With multiple instances of resource types, a cycle alone is not always sufficient to prove deadlock.

---

## 6. Wait-for Graph (WFG)

A Wait-for Graph simplifies the relationships by showing only threads.

If T1 is waiting for a resource currently held by T2, add:

```text
T1 → T2
```

For the two-thread example:

```text
T1 → T2
T2 → T1
```

This is a cycle.

For a three-thread example:

```text
T1 → T2
T2 → T3
T3 → T1
```

This is also a cycle.

For ThreadScope, the intended approach is to build a wait-for graph from tracked lock ownership and waiting relationships, then detect cycles in that graph.

---

## 7. Adjacency list in C++

An adjacency list stores the outgoing neighbours of each node.

For three threads:
- 0 represents T1.
- 1 represents T2.
- 2 represents T3.

```cpp
vector<vector<int>> graph(3);

graph[0].push_back(1); // T1 -> T2
graph[1].push_back(2); // T2 -> T3
graph[2].push_back(0); // T3 -> T1
```

`graph[node]` contains the nodes that the current node points to.

---

## 8. DFS cycle detection in a directed graph

The implementation uses two arrays:

```cpp
vector<int> visited(n, 0);
vector<int> path(n, 0);
```

- `visited[node] == 1`: the node has been visited at some point.
- `path[node] == 1`: the node is currently in the active DFS recursion path.

A previously visited node does not automatically mean there is a cycle. A cycle is detected when DFS reaches a node that is already in the **current recursion path**.

### DFS function

```cpp
bool dfs(int node, vector<vector<int>>& graph,
         vector<int>& visited, vector<int>& path) {

    visited[node] = 1;
    path[node] = 1;

    for (int neighbor : graph[node]) {

        if (!visited[neighbor]) {
            if (dfs(neighbor, graph, visited, path)) {
                return true;
            }
        }
        else if (path[neighbor]) {
            return true;
        }
    }

    path[node] = 0;
    return false;
}
```

### Important lines

1. `visited[node] = 1;` marks that the node has been explored.
2. `path[node] = 1;` marks that the node is on the current DFS path.
3. The loop explores every outgoing neighbour.
4. If a neighbour is unvisited, DFS explores it recursively.
5. If a neighbour is already in `path`, DFS has found a back edge to the current path, so a directed cycle exists.
6. `path[node] = 0;` removes the node from the active path when its exploration finishes.

### Check every disconnected component

```cpp
bool containsCycle(vector<vector<int>>& graph) {

    int n = graph.size();

    vector<int> visited(n, 0);
    vector<int> path(n, 0);

    for (int i = 0; i < n; i++) {
        if (!visited[i]) {
            if (dfs(i, graph, visited, path)) {
                return true;
            }
        }
    }

    return false;
}
```

The outer loop matters because a graph may have disconnected components. Starting DFS only from node 0 could miss a cycle in another component.

### Complexity

- **Time:** O(V + E), where V is the number of vertices and E is the number of directed edges.
- **Space:** O(V) for the visited array, active-path array, and recursion stack.

---

## 9. Complete standalone cycle detector

File: `cycle_detection.cpp`

```cpp
#include <iostream>
#include <vector>
using namespace std;

bool dfs(int node, vector<vector<int>>& graph,
         vector<int>& visited, vector<int>& path) {

    visited[node] = 1;
    path[node] = 1;

    for (int neighbor : graph[node]) {

        if (!visited[neighbor]) {
            if (dfs(neighbor, graph, visited, path)) {
                return true;
            }
        }
        else if (path[neighbor]) {
            return true;
        }
    }

    path[node] = 0;
    return false;
}

bool containsCycle(vector<vector<int>>& graph) {

    int n = graph.size();

    vector<int> visited(n, 0);
    vector<int> path(n, 0);

    for (int i = 0; i < n; i++) {
        if (!visited[i]) {
            if (dfs(i, graph, visited, path)) {
                return true;
            }
        }
    }

    return false;
}

int main() {

    // Test 1: Graph with a cycle
    vector<vector<int>> graph1(3);

    graph1[0].push_back(1); // T1 -> T2
    graph1[1].push_back(2); // T2 -> T3
    graph1[2].push_back(0); // T3 -> T1

    cout << "Graph 1: ";

    if (containsCycle(graph1))
        cout << "Cycle detected\n";
    else
        cout << "No cycle\n";


    // Test 2: Graph without a cycle
    vector<vector<int>> graph2(4);

    graph2[0].push_back(1); // T1 -> T2
    graph2[1].push_back(2); // T2 -> T3
    graph2[2].push_back(3); // T3 -> T4

    cout << "Graph 2: ";

    if (containsCycle(graph2))
        cout << "Cycle detected\n";
    else
        cout << "No cycle\n";

    return 0;
}
```

Compile and run from the `cpp-engine` directory:

```bash
g++ -std=c++17 cycle_detection.cpp -o cycle_detection -pthread
```

PowerShell:

```powershell
.\cycle_detection
```

Expected output:

```text
Graph 1: Cycle detected
Graph 2: No cycle
```

These are the results obtained during the Day 5 exercise.

---

## 10. Controlled two-mutex deadlock demonstration

The demonstration uses two `timed_mutex` objects. Each thread acquires one mutex and then tries to acquire the other with a timeout.

The timeout is important: it prevents the demonstration from hanging permanently.

Core pattern:

```cpp
timed_mutex m1, m2;

// Thread 1
m1.lock();
if (m2.try_lock_for(chrono::seconds(2))) {
    m2.unlock();
}
m1.unlock();

// Thread 2
m2.lock();
if (m1.try_lock_for(chrono::seconds(2))) {
    m1.unlock();
}
m2.unlock();
```

The actual demonstration adds a short delay after each first lock so both threads have an opportunity to acquire their first mutex before requesting the second.

### Observed terminal output

```text
Thread 1 acquired Mutex 1
Thread 2 acquired Mutex 2
Thread 2 requesting Mutex 1
Thread 1 requesting Mutex 2
Thread 1 timed out waiting for Mutex 2
Thread 2 timed out waiting for Mutex 1
Program completed
```

Interpretation:
- T1 owns Mutex 1.
- T2 owns Mutex 2.
- T1 requests Mutex 2, which is held by T2.
- T2 requests Mutex 1, which is held by T1.
- Both timed waits expire.
- Each thread can then release its original mutex.
- The program terminates.

This is a controlled demonstration of a deadlock scenario. Because the timed waits eventually expire, it is not a permanent deadlock.

---

## 11. Important distinction: standalone graph vs real deadlock detection

The Day 5 DFS program receives a graph that is manually constructed in `main()`. It detects a cycle in that graph; it does not yet observe real mutex ownership or waiting threads.

The timed-mutex demonstration produces a real lock-contention scenario, but it does not automatically feed its ownership and waiting information into the DFS detector.

**Day 6 bridges these two pieces:** instrumented locks will record lock requests, acquisitions, and releases. The program can use that tracking to construct a wait-for graph dynamically.

---

## 12. Interview and revision questions

1. What is a deadlock?
2. State the four necessary conditions for deadlock.
3. Explain the difference between deadlock, starvation, and livelock.
4. What is a resource allocation graph?
5. What is the difference between a request edge and an assignment edge?
6. What is a wait-for graph?
7. If T1 waits for a mutex held by T2, which directed edge is added?
8. What is an adjacency list?
9. Why do we need both `visited` and `path` in directed DFS cycle detection?
10. Why does reaching a previously visited node not always mean a cycle?
11. Why must we check all unvisited vertices?
12. What are the time and space complexities of the DFS cycle detector?
13. Why did the deadlock demonstration use `timed_mutex` and `try_lock_for()`?
14. Does the standalone DFS program detect real mutex deadlocks yet?

### Short answers

1. Threads are blocked waiting for resources held by one another.
2. Mutual exclusion, hold and wait, no preemption, circular wait.
3. Deadlock means blocked circular waiting; starvation means one thread is repeatedly denied; livelock means active threads make no useful progress.
4. A directed graph of threads, resources, requests, and allocations.
5. Thread → Resource is a request; Resource → Thread is an assignment.
6. A graph containing only threads, with an edge from a waiting thread to the thread holding the requested resource.
7. T1 → T2.
8. A representation in which each vertex stores its outgoing neighbours.
9. `visited` tracks explored nodes; `path` tracks the active recursion path.
10. The node must be in the current DFS recursion path to indicate a directed back edge.
11. A disconnected component may contain a cycle.
12. O(V + E) time and O(V) auxiliary space.
13. To demonstrate lock contention while ensuring the program eventually terminates.
14. No. It detects cycles only in the graph supplied to it. Dynamic lock tracking is a Day 6 goal.

---

## 13. Day 5 completion checklist

- [x] Understood deadlock and the four necessary conditions.
- [x] Distinguished deadlock, starvation, and livelock.
- [x] Learned resource allocation graphs.
- [x] Learned wait-for graphs.
- [x] Represented a directed graph using an adjacency list.
- [x] Implemented DFS cycle detection.
- [x] Tested a cyclic graph.
- [x] Tested an acyclic graph.
- [x] Ran the controlled two-mutex timeout demonstration.

**Day 5 complete.**

Next: **Day 6 — Instrumented locks, lock lifecycle events, ownership tracking, and dynamic wait-for graph construction.**
