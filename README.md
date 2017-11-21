This project provides basic building blocks for **asynchronous (reactive) communication** between threads. It has the ambition to become fully available on **all major platforms** (although part of it is currently limited to Linux). It has the ambition to be suitable for **real-time applications**: it aims to provide wait-free or at least lock-free progress guarantees.

It is similar in purpose to the Microsoft's Parallel Patterns Library which is limited to Windows, and actor frameworks like [CAF](https://actor-framework.org/) which provide higher-level abstractions.

## Queues

- `SPSC_Queue`: A wait-free single-producer-single-consumer queue. Most efficient.
- `MPMC_Queue`: A wait-free multi-producer-multi-consumer queue.
- `MPSC_Queue`: A wait-free multi-producer-single-consumer queue. More efficient than the MPMC queue.

All queues have the same interface, they only differ in how many producer and consumer threads can use them at the same time.

Example:

    #include "concurrency/spsc_queue.h"
    #include <thread>
    #include <chrono>
    #include <iostream>

    using namespace Reactive;
    using namespace std;

    int main()
    {
        // Queue with capacity 100 items
        SPSC_Queue<int> q(100);

        // Producer thread
        thread t1([&]()
        {
            for (int i = 0; i < 5; ++i)
            {
                q.push(i);
                this_thread::sleep_for(chrono::milliseconds(500));
            }
        });

        // Consumer thread
        thread t2([&]()
        {
            for (int i = 0; i < 5; ++i)
            {
                int v;
                while(!q.pop(v))
                    this_thread::sleep_for(chrono::milliseconds(100));
                cout << v << endl;
            }
        });

        t1.join(); t2.join();
    }

## Streams

Lock-free stream producers and consumers connected via queues in many-to-many patterns.

Each producer can be connected to multiple consumers and the other way around.

Each consumer has a MPSC queue from which it pops and to which producers push.

Example:

    #include "concurrency/streams.h"
    #include <thread>
    #include <chrono>
    #include <iostream>

    using namespace Reactive;
    using namespace std;

    int main()
    {
        Stream_Producer<int> producer;
        Stream_Consumer<int> consumer(5);

        // Connect producer and consumer. This is thread-safe.
        connect(producer, consumer);

        thread t1([&]()
        {
            for (int i = 0; i < 5; ++i)
            {
                producer.push(i);
                this_thread::sleep_for(chrono::milliseconds(500));
            }
        });

        thread t2([&]()
        {
            for (int i = 0; i < 5; ++i)
            {
                int v;
                while(!consumer.pop(v))
                    this_thread::sleep_for(chrono::milliseconds(100));
                cout << v << endl;
            }
        });

        t1.join(); t2.join();

        // Disconnect producer and consumer. This is thread-safe.
        disconnect(producer, consumer);

        // When a producer or consumer is destroyed,
        // it is automatically disconnected, in a thread-safe manner.
    }

## Events

An interface for waiting on multiple events. Events are provided by:

- Queue classes (presented above)
- `Timer` class (single-shot or periodic timer)
- `File_Event` class (from any file descriptor)
- `File` class (high-level interface to open, read and write files in the filesystem)
- `Signal` class (internally generated events)

The implementation is specific to the operating system. This is currently only implemented for Linux.

Example:

    #include "concurrency/events.h"
    #include "concurrency/timer.h"
    #include "concurrency/spsc_queue.h"
    #include <thread>
    #include <chrono>
    #include <iostream>

    using namespace Reactive;
    using namespace std;

    int main()
    {
        Timer timer;
        SPSC_Queue<int> q(100);

        // Start periodic timer
        timer.start(chrono::milliseconds(1000), true);

        thread t1([&]()
        {
            for (int i = 0; i < 10; ++i)
            {
                q.push(i);
                this_thread::sleep_for(chrono::milliseconds(300));
            }
        });

        Event_Reactor reactor;

        reactor.subscribe(timer.event(), [&]()
        {
            cout << "Timer" << endl;
        });

        reactor.subscribe(q.write_event(), [&]()
        {
            int v;
            q.pop(v);
            cout << "Value: " << v << endl;
            if (v >= 9)
            {
                cout << "Quit." << endl;
                reactor.quit();
            }
        });

        reactor.run(Event_Reactor::WaitUntilQuit);

        t1.join();
    }
