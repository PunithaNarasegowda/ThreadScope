
#include <iostream>
#include <mutex>
#include <thread>
#include <chrono>
#include <unordered_map>
#include <vector>
#include <functional>
#include <condition_variable>
#include <string>

using namespace std;

// ==========================================
// 1. EVENT LOGGING
// ==========================================

mutex logMutex;

void logEvent(int threadId, const string& eventType) {
    lock_guard<mutex> lock(logMutex);

    cout << "Thread " << threadId
         << " | " << eventType << '\n';
}

// ==========================================
// 2. DEADLOCK TRACKER
// ==========================================

class DeadlockTracker {
private:
    mutex trackingMutex;

    // Mutex ID -> Owner thread ID
    unordered_map<int, int> ownership;

    // Waiting thread ID -> Requested mutex ID
    unordered_map<int, int> waiting;

    bool dfs(
        int node,
        unordered_map<int, vector<int>>& graph,
        unordered_map<int, int>& visited
    ) {
        // 1 = currently visiting
        // 2 = completely visited

        visited[node] = 1;

        for (int neighbor : graph[node]) {

            if (visited[neighbor] == 1) {
                return true;
            }

            if (visited[neighbor] == 0) {
                if (dfs(neighbor, graph, visited)) {
                    return true;
                }
            }
        }

        visited[node] = 2;
        return false;
    }

public:

    // Record a lock request
    void recordWaiting(int threadId, int mutexId) {
        lock_guard<mutex> lock(trackingMutex);

        waiting[threadId] = mutexId;
    }

    // Record successful acquisition
    void recordAcquired(int threadId, int mutexId) {
        lock_guard<mutex> lock(trackingMutex);

        waiting.erase(threadId);
        ownership[mutexId] = threadId;
    }

    // Record release
    void recordReleased(int threadId, int mutexId) {
        lock_guard<mutex> lock(trackingMutex);

        auto it = ownership.find(mutexId);

        if (it != ownership.end() && it->second == threadId) {
            ownership.erase(it);
        }
    }

    // Remove a request after timeout
    void cancelWaiting(int threadId) {
        lock_guard<mutex> lock(trackingMutex);

        waiting.erase(threadId);
    }

    // Build graph and detect cycles
    bool detectCycle() {

        unordered_map<int, vector<int>> graph;

        {
            lock_guard<mutex> lock(trackingMutex);

            for (const auto& entry : waiting) {

                int waitingThread = entry.first;
                int requestedMutex = entry.second;

                auto owner = ownership.find(requestedMutex);

                if (owner != ownership.end()) {

                    int ownerThread = owner->second;

                    if (waitingThread != ownerThread) {

                        graph[waitingThread].push_back(ownerThread);
                        graph[ownerThread];
                    }
                }
            }
        }

        // Display graph
        {
            lock_guard<mutex> lock(logMutex);

            cout << "\n--- WAIT-FOR GRAPH ---\n";

            for (const auto& entry : graph) {

                for (int neighbor : entry.second) {

                    cout << "Thread " << entry.first
                         << " -> Thread " << neighbor << '\n';
                }
            }
        }

        // DFS cycle detection
        unordered_map<int, int> visited;

        for (const auto& entry : graph) {

            int node = entry.first;

            if (visited[node] == 0) {

                if (dfs(node, graph, visited)) {

                    lock_guard<mutex> lock(logMutex);

                    cout << "DEADLOCK DETECTED: Cycle found!\n";

                    return true;
                }
            }
        }

        cout << "No cycle detected.\n";
        return false;
    }
};

DeadlockTracker tracker;

// ==========================================
// 3. INSTRUMENTED MUTEX
// ==========================================

class InstrumentedMutex {
private:
    timed_mutex resourceMutex;
    int mutexId;

public:

    InstrumentedMutex(int id) : mutexId(id) {}

    // Normal blocking acquisition
    void lock(int threadId) {

        logEvent(threadId, "LOCK_REQUESTED");

        tracker.recordWaiting(threadId, mutexId);

        resourceMutex.lock();

        tracker.recordAcquired(threadId, mutexId);

        logEvent(threadId, "LOCK_ACQUIRED");
    }

    // Timed acquisition
    bool lockFor(
        int threadId,
        chrono::milliseconds timeout
    ) {

        logEvent(threadId, "LOCK_REQUESTED");

        tracker.recordWaiting(threadId, mutexId);

        tracker.detectCycle();

        if (resourceMutex.try_lock_for(timeout)) {

            tracker.recordAcquired(threadId, mutexId);

            logEvent(threadId, "LOCK_ACQUIRED");

            return true;
        }

        tracker.cancelWaiting(threadId);

        logEvent(threadId, "LOCK_TIMEOUT");

        return false;
    }

    // Release mutex
    void unlock(int threadId) {

        tracker.recordReleased(threadId, mutexId);

        logEvent(threadId, "LOCK_RELEASED");

        resourceMutex.unlock();
    }
};

// ==========================================
// 4. SYNCHRONIZATION BARRIER
// ==========================================

mutex barrierMutex;
condition_variable barrierCV;

int readyThreads = 0;

void waitForBothThreads() {

    unique_lock<mutex> lock(barrierMutex);

    readyThreads++;

    barrierCV.notify_all();

    barrierCV.wait(lock, [] {
        return readyThreads == 2;
    });
}

// ==========================================
// 5. CONTROLLED DEADLOCK DEMONSTRATION
// ==========================================

InstrumentedMutex m1(1);
InstrumentedMutex m2(2);

void thread1() {

    m1.lock(1);

    cout << "Thread 1 acquired Mutex 1\n";

    waitForBothThreads();

    cout << "Thread 1 requesting Mutex 2\n";

    if (m2.lockFor(1, chrono::seconds(2))) {

        cout << "Thread 1 acquired Mutex 2\n";

        m2.unlock(1);
    }

    m1.unlock(1);
}

void thread2() {

    m2.lock(2);

    cout << "Thread 2 acquired Mutex 2\n";

    waitForBothThreads();

    cout << "Thread 2 requesting Mutex 1\n";

    if (m1.lockFor(2, chrono::seconds(2))) {

        cout << "Thread 2 acquired Mutex 1\n";

        m1.unlock(2);
    }

    m2.unlock(2);
}

// ==========================================
// 6. NORMAL LOCKING DEMONSTRATION
// ==========================================

InstrumentedMutex normalMutex(3);

void normalWorker(int threadId) {

    if (normalMutex.lockFor(
        threadId,
        chrono::seconds(3)
    )) {

        cout << "Thread " << threadId
             << " is working...\n";

        this_thread::sleep_for(
            chrono::milliseconds(100)
        );

        normalMutex.unlock(threadId);
    }
}

// ==========================================
// 7. MAIN FUNCTION
// ==========================================

int main() {

    cout << "\n========== DEADLOCK TEST ==========\n";

    thread t1(thread1);
    thread t2(thread2);

    t1.join();
    t2.join();

    cout << "\n========== NORMAL LOCKING TEST ==========\n";

    thread t3(normalWorker, 3);
    thread t4(normalWorker, 4);

    t3.join();
    t4.join();

    cout << "\nProgram completed successfully.\n";

    return 0;
}
