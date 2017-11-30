Stitch {#mainpage}
======

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

Connections solve the problem of passing a reference to a shared object to multiple threads and managing the shared object's lifetime. A connection is an abstraction for the sharing of an object between two threads. When two threads are connected, they get access to the same shared object. Shared objects are created and destroyed as needed.

Read more about @subpage connections ...

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
[Actor Model]: https://en.wikipedia.org/wiki/Actor_model
