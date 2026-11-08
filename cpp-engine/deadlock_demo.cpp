#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>

using namespace std;

timed_mutex m1, m2;

void thread1() {
    m1.lock();
    cout << "Thread 1 acquired Mutex 1\n";

    this_thread::sleep_for(chrono::milliseconds(100));

    cout << "Thread 1 requesting Mutex 2\n";

    if (m2.try_lock_for(chrono::seconds(2))) {
        cout << "Thread 1 acquired Mutex 2\n";
        m2.unlock();
    } else {
        cout << "Thread 1 timed out waiting for Mutex 2\n";
    }

    m1.unlock();
}

void thread2() {
    m2.lock();
    cout << "Thread 2 acquired Mutex 2\n";

    this_thread::sleep_for(chrono::milliseconds(100));

    cout << "Thread 2 requesting Mutex 1\n";

    if (m1.try_lock_for(chrono::seconds(2))) {
        cout << "Thread 2 acquired Mutex 1\n";
        m1.unlock();
    } else {
        cout << "Thread 2 timed out waiting for Mutex 1\n";
    }

    m2.unlock();
}

int main() {
    thread t1(thread1);
    thread t2(thread2);

    t1.join();
    t2.join();

    cout << "Program completed\n";

    return 0;
}