#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>
using namespace std;
mutex coutMutex;
void performTask(int id) {

    {
        lock_guard<mutex> lock(coutMutex);
        cout << "Thread " << id << " has started.\n";
    }

    this_thread::sleep_for(chrono::milliseconds(100 * id));

    {
        lock_guard<mutex> lock(coutMutex);
        cout << "Thread " << id << " has finished.\n";
    }
}

int main() {
    cout << "ThreadScope started.\n";

    thread t1(performTask, 1);
    thread t2(performTask, 2);
    thread t3(performTask, 3);
    thread t4(performTask, 4);
    thread t5(performTask, 5);
    thread t6(performTask, 6);

    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    t6.join();

    cout << "All threads have finished.\n";

    return 0;
}