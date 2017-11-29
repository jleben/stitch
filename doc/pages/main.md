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
Shared objects are created on-demand when connections between threads are established, and destroyed automatically when the connections that use them are broken. The same object can be shared between multiple connected threads.

The endpoints of connections are represented by the Server and Client classes. A Server owns a shared object and provides it to all Clients that connect to it. In addition, Clients can be connected directly to each other, in which case a new shared object is created for each one-to-one connection. Connections can be broken explicitly, or automatically whenever an endpoint is destroyed. A shared object is automatically destroyed when all connections using it cease to exist.

- [Server](@ref Stitch::Server)
- [Client](@ref Stitch::Client)

### Stream producers and consumers

Stitch has a couple handy subclasses based on Server and Client to model connections of stream producers and consumers. Stream_Consumer is a Server and Stream_Producer is a Client, with MPSC_Queue as the shared object type. This implements the flow of data in an actor model, where the queue represents the mailbox of the receiver of messages (consumer) and messages can arrive from multiple senders (producers).

- [Stream_Producer](@ref Stitch::Stream_Producer)
- [Stream_Consumer](@ref Stitch::Stream_Consumer)

Events
------
