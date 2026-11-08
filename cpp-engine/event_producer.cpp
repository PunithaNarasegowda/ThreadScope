#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>
#include <string>

using namespace std;
using namespace chrono;

mutex outputMutex;

auto startTime = steady_clock::now();

long long getTimestamp() {
    auto now = steady_clock::now();

    return duration_cast<milliseconds>(
        now - startTime
    ).count();
}

void emitEvent(int threadId, string eventType) {

    lock_guard<mutex> lock(outputMutex);

    cout << "{"
         << "\"threadId\":" << threadId << ","
         << "\"eventType\":\"" << eventType << "\","
         << "\"timestamp\":" << getTimestamp()
         << "}"
         << endl;
}

void worker(int threadId) {

    emitEvent(threadId, "START");

    this_thread::sleep_for(milliseconds(500));

    emitEvent(threadId, "WORKING");

    this_thread::sleep_for(milliseconds(500));

    emitEvent(threadId, "FINISH");
}

int main() {

    thread t1(worker, 1);
    thread t2(worker, 2);
    thread t3(worker, 3);

    t1.join();
    t2.join();
    t3.join();

    return 0;
}