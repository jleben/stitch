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

Connections solve the problem of passing a reference to a shared object to multiple threads and managing the object's lifetime.
Shared objects are created on-demand when connections between threads are established, and destroyed automatically when the connections that use them are destroyed.

The endpoints of connections are represented by the Server and Client classes:

- [Server](@ref Stitch::Server)
- [Client](@ref Stitch::Client)

Connections are managed using the following functions:

- [connect](@ref Stitch::connect)
- [disconnect](@ref Stitch::disconnect)

A connection is also destroyed automatically when either of its endpoints is destroyed.

A Server owns a shared object and provides this same object to all Clients that connect to it. The lifetime of the shared object is equal to the lifetime of the Server.

When two Clients are connected, a shared object is associated specifically with this connection, and it is destroyed as soon as they are disconnected or either of them is destroyed.

### Stream producers and consumers

Stitch has a couple handy subclasses of Server and Client to model connections between stream producers and consumers:

- [Stream_Producer](@ref Stitch::Stream_Producer)
- [Stream_Consumer](@ref Stitch::Stream_Consumer)

Stream_Consumer is a Server and Stream_Producer is a Client, with MPSC_Queue as the shared object type.

This implements the flow of data in an actor model: the queue represents the mailbox of the receiver of messages (consumer) and messages can arrive from multiple senders (producers).


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
