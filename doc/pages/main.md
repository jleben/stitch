Introduction {#mainpage}
============

Stitch is a C++ library that provides basic building blocks for communication between threads. It aims to be suitable for real-time applications; all the time-critical operations have at least the lock-free progress guarantee and low average time complexity.


Shared Data Structures
----------------------

The foundation of thread communication are data structures that can be collaboratively used by multiple threads:

- [Waitfree_SPSC_Queue](@ref Stitch::Waitfree_SPSC_Queue): Wait-free single-producer-single-consumer bounded-size queue. Most efficient.
- [Waitfree_MPSC_Queue](@ref Stitch::Waitfree_MPSC_Queue): Wait-free multi-producer-single-consumer bounded-size queue. More efficient than the wait-free MPMC queue.
- [Waitfree_MPMC_Queue](@ref Stitch::Waitfree_MPMC_Queue): Wait-free multi-producer-multi-consumer bounded-size queue.
- [Lockfree_MPMC_Queue](@ref Stitch::Lockfree_MPMC_Queue): Lock-free multi-producer-multi-consumer bounded-size queue. More efficient than the wait-free MPSC and MPMC queues.
- [SPMC_Atom](@ref Stitch::SPMC_Atom): Lock-free single-writer-multi-reader atomic value of any trivially copyable type (regardless of size). More efficient than the generic Atom.
- [Atom](@ref Stitch::Atom): Lock-free multi-writer-multi-reader atomic value of any type (regardless of size).
- [Set](@ref Stitch::Set): An unordered dynamically-sized set of items with lock-free iteration.


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
