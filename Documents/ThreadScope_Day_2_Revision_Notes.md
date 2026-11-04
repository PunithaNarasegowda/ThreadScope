# ThreadScope --- Day 2 Revision Notes

## Race Conditions, Mutexes, Critical Sections, and Deadlocks

> **Goal of Day 2:** Understand why shared data needs synchronization,
> how a mutex protects it, and how incorrect lock ordering can cause
> deadlocks.

------------------------------------------------------------------------

## 1. Why shared data can be a problem

When multiple threads access the same variable, they may interfere with
one another if at least one thread modifies it and the accesses are not
properly synchronized.

Example of shared data:

``` cpp
long long counter = 0;
```

If six threads all update this same variable, their operations can
overlap.

### The `counter++` problem

It is tempting to think that:

``` cpp
counter++;
```

is one indivisible operation. Conceptually, an ordinary increment
involves:

1.  Read the current value.
2.  Add one.
3.  Write the updated value back.

Suppose `counter` is initially `5` and two threads increment it:

  Step   Thread T1      Thread T2        Counter
  ------ -------------- -------------- ---------
  1      Reads 5                               5
  2                     Reads 5                5
  3      Calculates 6                          5
  4                     Calculates 6           5
  5      Writes 6                              6
  6                     Writes 6               6

Two increments were attempted, but the final value is `6`, not `7`. One
update has been lost. This is called a **lost update**.

### Expected counter example

If six threads each increment a shared counter 1,000 times, the expected
mathematical total is:

``` text
6 threads × 1,000 increments = 6,000
```

But without synchronization, the program is not guaranteed to produce
6,000.

**Important C++ rule:** Concurrent unsynchronized reads/writes that
conflict on the same memory location create a **data race**. A data race
causes **undefined behavior** in C++. Do not assume the result will
simply be a slightly smaller number.

### Race condition vs. data race

-   **Race condition:** A broader concurrency problem where the outcome
    depends on the timing or ordering of operations.
-   **Data race:** A specific C++ memory-model violation involving
    conflicting unsynchronized accesses to the same memory location, at
    least one of which is a write.

------------------------------------------------------------------------

## 2. What is a mutex?

A **mutex** means *mutual exclusion*. It is a synchronization mechanism
that allows only one thread at a time to own a particular mutex.

A mutex protects shared data when all relevant accesses follow the same
locking discipline.

Example:

``` cpp
#include <iostream>
#include <thread>
#include <mutex>
using namespace std;

long long counter = 0;
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

    cout << "Final counter: " << counter << '\n';
    return 0;
}
```

Expected output:

``` text
Final counter: 6000
```

### How `lock()` and `unlock()` work

``` cpp
mtx.lock();

// Critical section
counter++;

mtx.unlock();
```

-   `lock()` attempts to acquire the mutex.
-   If another thread already owns it, the calling thread blocks until
    it can acquire the mutex.
-   The thread that owns the mutex executes the protected section.
-   `unlock()` releases the mutex, making it available for another
    thread to acquire.

**Important:** Unlocking a mutex does not guarantee that a particular
waiting thread gets it next. `std::mutex` does not promise FIFO
acquisition order. The scheduling and mutex implementation determine
which waiting thread acquires it.

------------------------------------------------------------------------

## 3. What is a critical section?

A **critical section** is the part of a program that accesses shared
data or a shared resource and therefore needs protection.

In this example:

``` cpp
mtx.lock();

counter++;  // Critical section

mtx.unlock();
```

The increment is the critical section.

### Do mutexes make the whole program sequential?

No. The six threads can still run concurrently.

Only the code protected by the same mutex is restricted so that one
thread at a time can execute that protected section.

  -----------------------------------------------------------------------
  Activity                            What happens?
  ----------------------------------- -----------------------------------
  Six worker threads exist            They can execute concurrently.

  A thread tries to lock an           It blocks while waiting.
  already-owned mutex                 

  A thread owns the mutex             It can execute the protected
                                      critical section.

  The mutex is unlocked               Another thread can acquire it.
  -----------------------------------------------------------------------

A mutex does not make the entire program sequential. It provides mutual
exclusion around the protected resource.

------------------------------------------------------------------------

## 4. What if we forget `unlock()`?

Consider:

``` cpp
mtx.lock();

counter++;

// Forgot mtx.unlock()
```

The mutex remains locked. Other threads that try to acquire it may wait
indefinitely.

This can cause the program to hang. If the owning thread exits the
relevant function without releasing the mutex, the mutex is still
locked; it is not automatically released merely because the function
ended.

This is one reason manual `lock()` and `unlock()` calls can be
error-prone.

------------------------------------------------------------------------

## 5. Safer locking with `std::lock_guard`

C++ provides `std::lock_guard` to manage mutex ownership automatically
using **RAII** (Resource Acquisition Is Initialization).

``` cpp
void incrementCounter() {
    for (int i = 0; i < 1000; i++) {
        lock_guard<mutex> lock(mtx);
        counter++;
    }
}
```

What happens:

1.  A `lock_guard` object is created.
2.  Its constructor locks the mutex.
3.  The protected code executes.
4.  When the `lock_guard` goes out of scope, its destructor unlocks the
    mutex automatically.

The mutex is held only for the lifetime of the `lock_guard`.

### Why is this safer?

With manual locking, an early return or exception might skip the
`unlock()` call. `lock_guard` releases the mutex automatically when its
scope ends.

**Good practice:** Keep critical sections short. Do not hold a mutex
while sleeping or doing unrelated work.

------------------------------------------------------------------------

## 6. What is a deadlock?

A **deadlock** occurs when two or more threads are permanently blocked
because each is waiting for a resource held by another thread in the
group.

### Example with two mutexes

There are two mutexes:

``` cpp
mutex mutex1;
mutex mutex2;
```

Imagine this sequence:

-   T1 locks `mutex1`.
-   T2 locks `mutex2`.
-   T1 tries to lock `mutex2`, but T2 owns it. T1 waits.
-   T2 tries to lock `mutex1`, but T1 owns it. T2 waits.

Now:

-   T1 is waiting for T2 to release `mutex2`.
-   T2 is waiting for T1 to release `mutex1`.
-   Neither can proceed to release the mutex it already holds.

This creates a circular wait, and both threads are stuck.

### Deadlock example in C++

This code demonstrates a possible deadlock. The exact result depends on
scheduling, so it may not deadlock on every run.

``` cpp
#include <iostream>
#include <thread>
#include <mutex>
using namespace std;

mutex mutex1;
mutex mutex2;

void task1() {
    lock_guard<mutex> lock1(mutex1);
    cout << "T1 acquired mutex1\n";

    lock_guard<mutex> lock2(mutex2);
    cout << "T1 acquired mutex2\n";
}

void task2() {
    lock_guard<mutex> lock1(mutex2);
    cout << "T2 acquired mutex2\n";

    lock_guard<mutex> lock2(mutex1);
    cout << "T2 acquired mutex1\n";
}

int main() {
    thread t1(task1);
    thread t2(task2);

    t1.join();
    t2.join();

    return 0;
}
```

If the threads acquire their first mutexes before either can acquire the
second, they can deadlock. If one thread gets both mutexes first, it may
finish without deadlocking.

**Safety note:** A deadlocked program may never reach the `join()`
calls' completion. Run deadlock experiments with a timeout or in a
process you can safely terminate.

------------------------------------------------------------------------

## 7. The four necessary conditions for deadlock

The classic Coffman conditions describe four conditions that must all be
present for a deadlock to occur:

  -----------------------------------------------------------------------
  Condition               Meaning                 Example
  ----------------------- ----------------------- -----------------------
  1\. Mutual Exclusion    A resource can be held  Only one thread can own
                          by only one thread at a `mutex1`.
                          time.                   

  2\. Hold and Wait       A thread holds one      T1 holds `mutex1` and
                          resource while waiting  waits for `mutex2`.
                          for another.            

  3\. No Preemption       A resource cannot       T2 cannot forcibly take
                          simply be taken away;   `mutex1` from T1.
                          the owner must release  
                          it.                     

  4\. Circular Wait       A circular chain exists T1 waits for T2, and T2
                          in which each thread    waits for T1.
                          waits for a resource    
                          held by the next.       
  -----------------------------------------------------------------------

For the classic deadlock scenario, all four conditions must hold
simultaneously.

------------------------------------------------------------------------

## 8. Deadlock prevention: consistent lock ordering

One simple way to prevent the circular-wait condition is to establish a
global lock order.

For example, require every thread to acquire:

1.  `mutex1` first.
2.  `mutex2` second.

Both threads follow the same order:

``` cpp
void task1() {
    lock_guard<mutex> lock1(mutex1);
    lock_guard<mutex> lock2(mutex2);

    // Work requiring both mutexes
}

void task2() {
    lock_guard<mutex> lock1(mutex1);
    lock_guard<mutex> lock2(mutex2);

    // Work requiring both mutexes
}
```

Why does this prevent the circular wait in this example?

-   A thread cannot hold `mutex2` while waiting for `mutex1`, because it
    must acquire `mutex1` first.
-   A thread waiting for `mutex1` does not yet hold `mutex2`.
-   Therefore, the circular chain where one thread holds `mutex1` and
    waits for `mutex2` while another holds `mutex2` and waits for
    `mutex1` cannot form.

This is called **consistent lock ordering**.

### Another option: `std::lock()`

C++ provides `std::lock()` to lock multiple mutexes together while
avoiding the deadlock risk associated with manually acquiring them in
conflicting orders.

Example:

``` cpp
#include <mutex>
using namespace std;

mutex mutex1;
mutex mutex2;

void doWork() {
    unique_lock<mutex> lock1(mutex1, defer_lock);
    unique_lock<mutex> lock2(mutex2, defer_lock);

    lock(lock1, lock2);

    // Work requiring both mutexes
}
```

-   `defer_lock` creates the `unique_lock` objects without locking their
    mutexes immediately.
-   `lock(lock1, lock2)` safely acquires both locks together.
-   The `unique_lock` objects release their locks automatically when
    they go out of scope.

This is an introduction; `unique_lock` and multi-mutex locking deserve a
deeper practical exercise later.

------------------------------------------------------------------------

## 9. Dining Philosophers Problem

The **Dining Philosophers Problem** is a classic operating-systems
example used to explain deadlock and resource contention.

Imagine philosophers sitting around a table. There is one fork between
each pair of philosophers. Each philosopher needs two forks to eat.

If every philosopher picks up the fork on their left and waits for the
fork on their right, nobody can proceed. Each holds one resource and
waits for another, creating circular wait.

This is similar to the two-thread, two-mutex example.

------------------------------------------------------------------------

## 10. Important comparisons

### `lock()` / `unlock()` vs. `lock_guard`

  -----------------------------------------------------------------------
  Manual locking                      `lock_guard`
  ----------------------------------- -----------------------------------
  You call `lock()` explicitly.       Constructor locks the mutex.

  You must remember to call           Destructor unlocks automatically.
  `unlock()`.                         

  Easier to make mistakes on early    Scope-based cleanup makes it safer.
  returns or exceptions.              
  -----------------------------------------------------------------------

### Race condition vs. deadlock

  -----------------------------------------------------------------------
  Race condition                      Deadlock
  ----------------------------------- -----------------------------------
  Result may depend on timing or      Threads become stuck waiting for
  operation order.                    resources.

  Can cause incorrect or              Can prevent the program from making
  unpredictable results.              progress.

  Example: unsynchronized shared      Example: T1 and T2 each hold a
  counter updates.                    mutex the other needs.
  -----------------------------------------------------------------------

### Mutex vs. critical section

-   **Mutex:** The synchronization object used to control access.
-   **Critical section:** The code region that needs protection.

------------------------------------------------------------------------

## 11. Interview-ready answers

**Q: Why can multiple threads updating one global variable cause a
problem?**

Because the threads share the same memory location. Without
synchronization, their read-modify-write operations can interfere with
one another, causing lost updates. In C++, conflicting unsynchronized
accesses can constitute a data race and cause undefined behavior.

**Q: What happens when T1 owns a mutex and T2 tries to lock it?**

T2 blocks while waiting for the mutex to become available. T1 can
continue through its critical section and release the mutex.

**Q: Does T2 automatically acquire the mutex immediately after T1
unlocks it?**

No. Unlocking makes the mutex available, but there is no general FIFO
guarantee about which waiting thread acquires it next.

**Q: Do mutexes make all threads execute sequentially?**

No. Threads can still execute concurrently. A mutex only provides mutual
exclusion for code protected by that mutex.

**Q: What happens if a mutex is never unlocked?**

Other threads waiting for it may remain blocked indefinitely, causing
the program to hang.

**Q: What is a deadlock?**

A deadlock occurs when threads are permanently blocked waiting for
resources held by one another, so none of them can proceed.

**Q: What are the four necessary conditions for deadlock?**

Mutual exclusion, hold and wait, no preemption, and circular wait.

**Q: How can consistent lock ordering prevent deadlock?**

If every thread acquires locks in the same predefined order, the
circular-wait pattern caused by acquiring the same locks in opposite
orders is prevented.

------------------------------------------------------------------------

## 12. Day 2 self-test

Try answering these without looking at the notes:

1.  Why can six threads accessing the same global variable cause a
    problem?
2.  Why is `counter++` not safe when multiple threads execute it without
    synchronization?
3.  What is a lost update?
4.  What is the difference between a race condition and a data race?
5.  When T1 owns a mutex, what happens when T2 tries to lock it?
6.  After T1 unlocks a mutex, is T2 guaranteed to acquire it next?
7.  Are the six threads in the mutex counter program concurrent or
    sequential? Explain.
8.  What is a critical section?
9.  What can happen if a thread forgets to unlock a mutex?
10. Why is `lock_guard` safer than manual `lock()` and `unlock()`?
11. What is a deadlock?
12. Explain the four necessary conditions for deadlock.
13. How does consistent lock ordering prevent circular wait?
14. What is the Dining Philosophers Problem?
15. What does `std::lock()` help us do?

------------------------------------------------------------------------

## 13. Day 2 learning checklist

-   [x] Shared variables and concurrent access
-   [x] Why `counter++` is not thread-safe
-   [x] Lost updates
-   [x] Race condition vs. data race
-   [x] Mutex and mutual exclusion
-   [x] `lock()` and `unlock()`
-   [x] Critical sections
-   [x] Concurrent threads vs. mutually exclusive sections
-   [x] Why forgetting to unlock a mutex is dangerous
-   [x] `lock_guard` and RAII
-   [x] Deadlock and circular wait
-   [x] Four necessary deadlock conditions
-   [x] Consistent lock ordering
-   [x] Introduction to `std::lock()` and `unique_lock`
-   [x] Dining Philosophers Problem

### Practical exercises to complete

1.  **`counter_race.cpp`:** Create six threads, each incrementing a
    shared counter one million times without synchronization. Observe
    that the program has undefined behavior. Results may vary, and even
    getting the expected total does not prove it is safe.
2.  **`counter_mutex.cpp`:** Protect the increment using a mutex and
    verify that the final result is 6,000,000.
3.  **`deadlock_demo.cpp`:** Create two threads and two mutexes, acquire
    the mutexes in opposite orders, and observe how deadlock can occur.
    Use a timeout so the experiment cannot hang indefinitely.
4.  **Lock-ordering exercise:** Change both threads to acquire `mutex1`
    before `mutex2` and explain why the circular wait is prevented.

------------------------------------------------------------------------

**Revision reminder:** For every multithreading example, ask yourself: -
Which data is shared? - Which thread currently owns the mutex? - Which
thread is waiting? - What is the critical section? - Can the threads
form a circular wait? - What guarantees that the shared data is accessed
safely?
