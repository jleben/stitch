The Stitch library provides basic building blocks for **communication** between threads. It has the ambition to be suitable for **real-time applications**: it aims to provide at least lock-free progress guarantees. It has the ambition to become fully available on **all major platforms** (although part of it is currently limited to Linux).

Stitch provides:

- Lock-free and wait-free thread-safe data structures (queue, atom, set, ...)
- Dynamically established communication channels between threads, with any user-provided type as the channel.
- Waiting for multiple internal or external events at once (elapsed time, files ready to read/write, internally generated events).

See the [documentation](http://webhome.csc.uvic.ca/~jleben/stitch/) for details.

See the [comparison with related software](#comparison-with-related-software) below.

Here is a couple examples...

## Shared data structures

    struct Data { int x, y, z; }

    Stitch::Atom<Data> d;
    Stitch::SPSC_Queue<int> q;

    thread t1([&]()
    {
        d.store({ 1, 2, 3 }); // Lock-free
        q.push(123); // Wait-free
    });

    thread t2([&]()
    {
        Data a = d.load(); // Lock-free
        int b;
        bool ok = q.pop(b); // Wait-free
    });


## Dynamic Connections

    using Mailbox = Stitch::MPSC_Queue<int>;
    Stitch::Server<Mailbox> server1(new Mailbox(100));
    Stitch::Server<Mailbox> server2(new Mailbox(100));
    Stitch::Client<Mailbox> client1;
    Stitch::Client<Mailbox> client2;

    Stitch::connect(client1, server1);
    Stitch::connect(client1, server2);
    Stitch::connect(client2, server2);

    thread t1([&]()
    {
        for(auto & destination : client1) // Lock-free
        {
            destination.push(123); // Wait-free
        }

        for(auto & destination : client2) // Lock-free
        {
            destination.push(321); // Wait-free
        }
    });

    thread t2([&]()
    {
        int data = server1->pop(); // Wait-free
    });

    thread t3([&]()
    {
        int data = server2->pop(); // Wait-free
    });

## Events

    Stitch::Timer timer;
    Stitch::Signal signal;

    timer.start(chrono::milliseconds(1000));

    thread t1([&]()
    {
        this_thread::sleep_for(chrono::milliseconds(500));
        signal.notify();
    });

    Stitch::Event_Reactor reactor;

    reactor.subscribe(timer.event(), [&]()
    {
        cout << "Timer" << endl;
    });

    reactor.subscribe(signal.event(), [&]()
    {
        cout << "Signal" << endl;
    });

    reactor.run(Stitch::Event_Reactor::Wait);

## Comparison with Related Software

These are some of our observations about related software which give reasons for the existence of this library. Please contact the authors of this text if any of this is incorrect or if something else should be added.

### Microsoft's Parallel Patterns Library

- Limited to Windows.
- No file events.
- No progress guarantees.

### C++ Actor Framework (CAF)

[Website](https://actor-framework.org/)

- A higher-level interface (the actor model) instead of basic building blocks.
- Does not specify detailed progress guarantees for each method of each class.

### libevent

[Website](http://libevent.org/)

- C instead of C++ (There exist C++ wrappers though).
- Does not provide a direct interface for waiting on events like our `wait(event)`.
- Does not provide a direct interface for internally generated events like our `Signal` class.
