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
