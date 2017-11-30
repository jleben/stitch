Introduction {#mainpage}
============

Stitch is a C++ library that provides basic building blocks for communication between threads. It aims to be suitable for real-time applications; all the time-critical operations have at least the lock-free progress guarantee and low average time complexity.


Shared Data Structures
----------------------

The foundation of thread communication are data structures that can be collaboratively used by multiple threads:

- [SPSC_Queue](@ref Stitch::SPSC_Queue): Wait-free single-producer-single-consumer bounded-size queue. More efficient than the `MPSC_Queue`.
- [MPSC_Queue](@ref Stitch::MPSC_Queue): Wait-free multi-producer-single-consumer bounded-size queue.
- [SPMC_Atom](@ref Stitch::SPMC_Atom): Lock-free single-writer-multi-reader value of any type (even a large data structure).
- [Set](@ref Stitch::Set): An unordered dynamically-sized set of items with lock-free iteration, and blocking insertion and removal.

Connections
-----------

Connections help with dynamically managing the communication of one thread with multiple other threads. They improve modularity: they enable writing classes which are agnostic of who they communicate with, while an external process manages their connections with multiple other objects.

Endpoints of connections are represented by the [Server](@ref Stitch::Server) and [Client](@ref Stitch::Client) classes,
and connections are managed by [connect](@ref Stitch::connect) and [disconnect](@ref Stitch::disconnect).

Read more on the @subpage connections page...

Example:

    Stitch::Server<atomic<int>> server1;
    Stitch::Server<atomic<int>> server2;
    Stitch::Client<atomic<int>> client;

    Stitch::connect(client, server1);
    Stitch::connect(client, server2);

    thread s1([&](){ server->store(1); });

    thread s2([&](){ server->store(2); });

    thread c([&]()
    {
        for(auto & data : client) { cout << data.load() << endl; }
    });

Events
------

An essential part of thread interaction is synchronization of threads in time. In addition, real-time applications need to wait and respond to data arriving from the outside world. The event system provides a convenient interface for waiting for and reacting to multiple types of events.

The [Event][] class is a generic representation of an event source. Instances of Event are provided by the following classes:

- [Signal](@ref Stitch::Signal): Events triggered by the application itself, used for signalling between threads.
- [Timer](@ref Stitch::Timer): Time events.
- [File_Event](@ref Stitch::File_Event): Events triggered when a file becomes ready for reading or writing.
- [File](@ref Stitch::File): A convenient class that provides File_Events as well as an interface for actual file reading and writing.

You can wait for a single event synchronously using the [wait](@ref Stitch::wait(const Stitch::Event&)) function. Waiting and reacting to multiple events is enabled by the [Event_Reactor](@ref Stitch::Event_Reactor) class.

[Event]: @ref Stitch::Event

Example:

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
