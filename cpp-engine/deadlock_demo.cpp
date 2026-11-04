#include <iostream>
#include <thread>
#include <mutex>
using namespace std;

mutex mutex1;
mutex mutex2;

void task1() {
    lock_guard<mutex> lock1(mutex1);

    cout << "Thread 1 acquired mutex1\n";

    lock_guard<mutex> lock2(mutex2);

    cout << "Thread 1 acquired mutex2\n";
}

void task2() {
    lock_guard<mutex> lock1(mutex2);

    cout << "Thread 2 acquired mutex2\n";

    lock_guard<mutex> lock2(mutex1);

    cout << "Thread 2 acquired mutex1\n";
}

int main() {
    thread t1(task1);
    thread t2(task2);

    t1.join();
    t2.join();

    return 0;
}