#include <iostream>
#include <thread>
using namespace std;

long long counter = 0;

void incrementCounter() {
    for (int i = 0; i < 1000000; i++) {
        counter++;
    }
}

int main() {

    thread t1(incrementCounter);
    thread t2(incrementCounter);
    thread t3(incrementCounter);
    thread t4(incrementCounter);
    thread t5(incrementCounter);
    thread t6(incrementCounter);

    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    t6.join();

    cout << "Final counter: " << counter << endl;

    return 0;
}