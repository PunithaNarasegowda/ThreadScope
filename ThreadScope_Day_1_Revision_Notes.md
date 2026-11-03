# ThreadScope --- Day 1 Revision Notes

These notes collect the concepts covered while starting ThreadScope:
processes, threads, concurrency, `std::thread`, `join()`, `detach()`,
race conditions, mutexes, and `lock_guard`.

## 1. Process and thread

A **process** is a running instance of a program. A process can contain
one or more threads.

A **thread** is an execution path within a process. Multiple threads
allow different tasks to make progress concurrently.

Example: A video editor may have: - Main thread: handles the user
interface. - Worker thread 1: processes video frames. - Worker thread 2:
processes audio. - Worker thread 3: saves the edited video.

Threads in the same process share the process's address space and
resources.

  -----------------------------------------------------------------------
  Process                             Thread
  ----------------------------------- -----------------------------------
  A running instance of a program.    An execution path within a process.

  Has its own process-level address   Shares the process's address space
  space and resources.                with other threads in that process.

  Can contain multiple threads.       Exists within a process.
  -----------------------------------------------------------------------

## 2. Main thread and worker threads

C++ program execution begins in `main()`. The thread executing `main()`
is called the **main thread**.

``` cpp
int main() {
    cout << "Hello World";
    return 0;
}
```

Additional threads created by the program are often called **worker
threads**. They can execute functions independently of the main thread.

## 3. Concurrency and parallelism

-   **Concurrency:** Multiple tasks make progress during overlapping
    periods. They may take turns using CPU time.
-   **Parallelism:** Multiple tasks literally execute at the same time,
    typically on different CPU cores.

A program can be concurrent even on a single CPU core. Actual parallel
execution depends on available hardware and scheduling.

## 4. Creating a thread with `std::thread`

``` cpp
thread t1(performTask, 1);
```

-   `thread`: C++ thread type (`std::thread`).
-   `t1`: the thread object.
-   `performTask`: function the new thread executes.
-   `1`: argument passed to the function.

Conceptually, the new thread executes `performTask(1)`. The function
runs on the new thread, not on the main thread.

### Example: six worker threads

``` cpp
#include <iostream>
#include <thread>
#include <chrono>
using namespace std;

void performTask(int id) {
    cout << "Thread " << id << " has started.\\n";
    this_thread::sleep_for(chrono::milliseconds(1000 * id));
    cout << "Thread " << id << " has finished.\\n";
}

int main() {
    cout << "ThreadScope started.\\n";

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

    cout << "All threads have finished.\\n";
    return 0;
}
```

`sleep_for()` pauses the calling thread for the specified duration. The
exact execution and output order is not guaranteed.

## 5. Why can output order change?

Thread scheduling is nondeterministic from the program's point of view.
The operating system decides when threads get CPU time. Scheduling,
other processes, and what each thread is doing can affect the order.

When multiple threads write to `cout`, parts of their messages may
appear interleaved. Complete multi-part messages are not automatically
printed as one indivisible unit.

To keep each message together, protect the complete print operation:

``` cpp
#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
using namespace std;

mutex coutMutex;

void performTask(int id) {
    {
        lock_guard<mutex> lock(coutMutex);
        cout << "Thread " << id << " has started.\\n";
    }

    this_thread::sleep_for(chrono::milliseconds(1000 * id));

    {
        lock_guard<mutex> lock(coutMutex);
        cout << "Thread " << id << " has finished.\\n";
    }
}
```

Keep the protected section short. Do not hold the output mutex while
sleeping.

## 6. What does `join()` do?

`join()` makes the thread that calls it wait until the associated worker
thread finishes.

``` cpp
thread t1(performTask, 1);
thread t2(performTask, 2);

t1.join();
t2.join();

cout << "Both tasks completed.\\n";
```

Important points: 1. `t1` and `t2` start when they are created. 2.
`t1.join()` blocks the calling thread (here, the main thread) until `t1`
finishes. 3. While the main thread waits for `t1`, `t2` can continue
executing. 4. After `t1.join()` returns, the main thread calls
`t2.join()` and waits for `t2`. 5. The final message runs only after
both joins return.

**The order of join calls does not determine the order in which worker
threads execute or finish.**

### Does `t1.join()` stop the other threads?

No. It blocks only the thread that calls `join()`. Other worker threads
can continue executing.

### Why is `join()` useful?

Suppose three threads download files and the main thread must combine
them afterward:

``` cpp
thread t1(downloadFile, 1);
thread t2(downloadFile, 2);
thread t3(downloadFile, 3);

t1.join();
t2.join();
t3.join();

combineFiles();
```

The joins ensure `combineFiles()` runs only after all downloads finish.
Joining also ensures that a thread has completed before its thread
object is destroyed.

## 7. What if we remove `join()`?

``` cpp
#include <iostream>
#include <thread>
#include <chrono>
using namespace std;

void performTask() {
    this_thread::sleep_for(chrono::seconds(3));
    cout << "Worker thread finished!\\n";
}

int main() {
    thread t1(performTask);
    cout << "Main thread finished!\\n";
    return 0;
}
```

When `main()` ends, local object `t1` is destroyed. If a `std::thread`
object is still **joinable** when its destructor runs, C++ calls
`std::terminate()`. It does not automatically wait for the worker.

A `std::thread` must be joined or detached before its destructor runs.

## 8. `join()` vs. `detach()`

  -----------------------------------------------------------------------
  `join()`                            `detach()`
  ----------------------------------- -----------------------------------
  Calling thread waits for the worker Calling thread does not wait for
  to finish.                          the worker.

  Thread object becomes non-joinable  Thread object becomes non-joinable
  after joining.                      after detaching.

  Useful when later work depends on   Worker continues independently of
  completion.                         the thread object.
  -----------------------------------------------------------------------

``` cpp
thread t1(performTask);
t1.detach();
```

**Caution:** Detached threads need careful lifetime and resource
management. The program must not destroy data that a detached thread is
still using. For the initial ThreadScope implementation, prefer
`join()`.

## 9. Race condition and shared data

A **race condition** is a concurrency problem where the result depends
on the timing or ordering of operations.

Suppose six threads each increment this shared variable 1,000 times:

``` cpp
int counter = 0;
counter++;
```

The expected mathematical total is:

`6 threads × 1,000 increments = 6,000`

But the result is not guaranteed without synchronization.

### Why is `counter++` unsafe?

It is tempting to think `counter++` is one indivisible operation.
Conceptually, it involves: 1. Read the current value. 2. Add one. 3.
Write the updated value.

For example, if `counter` starts at 5:

  Step   Thread T1      Thread T2        Counter
  ------ -------------- -------------- ---------
  1      Reads 5                               5
  2                     Reads 5                5
  3      Calculates 6                          5
  4                     Calculates 6           5
  5      Writes 6                              6
  6                     Writes 6               6

Two increments were attempted, but the final value is 6 rather than 7.
This is called a **lost update**.

### Important C++ rule

Concurrent unsynchronized increments of an ordinary `int` create a
**data race**, which causes **undefined behavior** in C++.

Do not assume the result will merely be a number slightly below 6,000.
The C++ standard does not define the program's behavior.

-   **Data race:** A specific C++ memory-model violation caused by
    conflicting unsynchronized memory accesses.
-   **Race condition:** A broader concurrency problem where behavior
    depends on timing or ordering.

## 10. What is a mutex?

A **mutex** (mutual exclusion) is a synchronization mechanism. It allows
only one thread at a time to hold a particular lock, protecting shared
data from simultaneous access by threads using that lock.

### Using `lock()` and `unlock()`

``` cpp
#include <iostream>
#include <thread>
#include <mutex>
using namespace std;

int counter = 0;
mutex mtx;

void incrementCounter() {
    for (int i = 0; i < 1000; i++) {
        mtx.lock();
        counter++;
        mtx.unlock();
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

    cout << "Final counter: " << counter << '\\n';
    return 0;
}
```

The mutex protects the increment: - A thread locks the mutex before
accessing the counter. - If another thread holds the mutex, the new
thread waits. - The thread updates the counter and unlocks the mutex. -
Another waiting thread can then acquire the lock.

The expected final result is `6000`.

## 11. Why use `lock_guard`?

Manual `lock()` and `unlock()` calls can be error-prone. If a function
returns early or an exception occurs before `unlock()`, the mutex might
remain locked.

`std::lock_guard` uses **RAII** (Resource Acquisition Is
Initialization):

``` cpp
void incrementCounter() {
    for (int i = 0; i < 1000; i++) {
        lock_guard<mutex> lock(mtx);
        counter++;
    }
}
```

What happens: 1. The `lock_guard` is created and locks the mutex. 2. The
protected operation executes. 3. When the `lock_guard` goes out of
scope, its destructor automatically unlocks the mutex.

Keep the critical section (the code protected by the mutex) as small as
practical.

## 12. Quick revision table

  -----------------------------------------------------------------------
  Concept                             Meaning
  ----------------------------------- -----------------------------------
  Process                             A running instance of a program.

  Thread                              An execution path within a process.

  Main thread                         The thread that begins executing at
                                      `main()`.

  Worker thread                       An additional thread created to
                                      perform a task.

  Concurrency                         Multiple tasks make progress over
                                      overlapping periods.

  Parallelism                         Multiple tasks execute at the same
                                      instant.

  `std::thread`                       C++ type used to represent and
                                      manage a thread.

  `join()`                            Makes the calling thread wait for
                                      the associated thread to finish.

  `detach()`                          Separates the thread's execution
                                      from the thread object.

  Race condition                      A timing/order-dependent
                                      concurrency problem.

  Data race                           Conflicting unsynchronized memory
                                      accesses; undefined behavior in
                                      C++.

  Mutex                               A lock that provides mutual
                                      exclusion.

  `lock_guard`                        RAII object that locks a mutex and
                                      unlocks it when it leaves scope.
  -----------------------------------------------------------------------

## 13. Self-test questions

Try answering these without looking at the notes:

1.  What is a thread, and why might a process need multiple threads?
2.  What happens when we write `thread t1(performTask, 1);`?
3.  Why do we call `join()` on worker threads?
4.  If we call `t1.join()` first, can `t2` still execute while the main
    thread waits for `t1`?
5.  Does the order of `join()` calls determine the order in which
    threads execute?
6.  Why might output order change between two runs?
7.  What is the main thread?
8.  What happens if a joinable `std::thread` object is destroyed?
9.  What is the difference between `join()` and `detach()`?
10. Why is `counter++` unsafe when multiple threads execute it without
    synchronization?
11. What is the difference between a race condition and a data race?
12. How does a mutex protect shared data?
13. Why is `lock_guard` generally safer than manually calling `lock()`
    and `unlock()`?

## 14. ThreadScope learning checkpoint

-   [x] Process vs. thread
-   [x] Main thread vs. worker threads
-   [x] Concurrency vs. parallelism
-   [x] Creating threads using `std::thread`
-   [x] Passing function arguments to a thread
-   [x] Nondeterministic execution and output ordering
-   [x] Understanding `join()`
-   [x] Understanding `detach()` and thread object lifetime
-   [x] Race conditions and data races
-   [x] Why `counter++` is not thread-safe
-   [x] Basic mutex usage
-   [x] Using `lock_guard`

### What comes next?

Build a shared-counter program: 1. Create a global `int counter = 0`. 2.
Create six worker threads. 3. Let each thread increment the counter
1,000 times. 4. Understand why unsynchronized increments are not safe.
5. Protect the increment with a mutex. 6. Join all six threads and
verify the final result is 6,000.

**Revision reminder:** Don't memorize only definitions. Be able to
explain which thread is executing, which thread is waiting, what data is
shared, and why synchronization is needed.
