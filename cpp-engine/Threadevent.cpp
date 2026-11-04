#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>
#include <vector>
#include <string>

using namespace std;
struct ThreadEvent {
    int threadId;
    string eventType;
    long long timestamp;
    int resourceId;
};
vector<ThreadEvent> events;
mutex eventsMutex;
void producer(ThreadEvent event) {
    lock_guard<mutex> lock(eventsMutex);
    events.push_back(event);
}

void generateEvents(int threadId) {
    for (const char* eventType : {"START", "WORKING", "FINISH"}) {
        ThreadEvent event;
        event.threadId = threadId;
        event.eventType = eventType;
        event.timestamp = chrono::duration_cast<chrono::milliseconds>(
            chrono::system_clock::now().time_since_epoch()).count();
        event.resourceId = threadId;
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
        cout << "Thread " << event.threadId
             << " | " << event.eventType
             << " | timestamp: " << event.timestamp
             << " | resource: " << event.resourceId << '\n';
    }

    return 0;
}
