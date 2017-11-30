Connections {#connections}
===========

Connections help with dynamically managing the communication of one thread with multiple other threads. They improve modularity: they enable writing classes which are agnostic of who they communicate with, while an external process manages their connections with multiple other objects.

A connection represents the sharing of an object between two threads.
However, each of the two threads only needs to interact with an object that represents one endpoint of the connection.
A third thread can connect and disconnect the endpoints, which automatically creates and destroyes the shared object as needed.

The endpoints of connections are represented by the Server and Client classes:

- [Server](@ref Stitch::Server)
- [Client](@ref Stitch::Client)

A Server owns a shared object and provides this same object to all Clients that connect to it. The lifetime of the shared object is equal to the lifetime of the Server.

In contrast, a Client gets access to a separate shared object for each of its connections. A connection to a Server provides it with the Server's shared object. A connection with another Client provides it with an individual shared object exclusive to this connection, and discarded when the Clients are disconnected.

Connections are managed using the following functions:

- [connect](@ref Stitch::connect)
- [disconnect](@ref Stitch::disconnect)

A connection is also destroyed automatically when either of its endpoints is destroyed.

This example demonstrates the effect of individual operations:

    struct Data
    {
        std::atomic<int> x
    };

    Stitch::Server<Data> server;
    Stitch::Client<Data> client1;
    Stitch::Client<Data> client2;

    Stitch::connect(client1, server);
    Stitch::connect(client2, server);

    // client1, client2 and server now share the same Data object owned by the server.

    Stitch::connect(client1, client2);

    // client1 and client2 now also share an additional Data object created for their connection.

    for (auto & data : client1)
    {
        // The loop is executed twice;
        // once for the server's object, and once for the object shared by the two clients.
        data.x = 2;
    }

    // Both shared objects now contain 2.

    server->x = 5;

    // The server's object now contains 5.

    for (auto & data : client2)
    {
        cout << data.x << endl;
    }

    // 2 and 5 are printed on the standard output, in an arbitrary order.

    Stitch::disconnect(client1, server);

    // The object shared between the two clients is now destroyed.
    // However, the server's object persists and is still shared with client2.


Connections are particularily useful when writing classes that can dynamically establish connections across multiple threads, while the communicating objects are agnostic of each other. For example:

    struct Data { std::atomic<int> x };

    struct Writer
    {
        Stitch::Server<Data> source;

        void run()
        {
            thread t([&]()
            {
                for(;;)
                {
                    data->x.fetch_add(1);
                    this_thread::sleep_for(chrono::milliseconds(1000));
                }
            });
        }
    }

    struct Reader
    {
        Stitch::Client<Data> sink;

        void run()
        {
            thread t([&]()
            {
                for(;;)
                {
                    for (auto & data : sink)
                    {
                        cout << data->x.load() << endl;
                        this_thread::sleep_for(chrono::milliseconds(200));
                    }
                }
            });
        }
    }

    Writer writer; writer.run();
    Reader reader1; reader1.run();
    Reader reader2; reader2.run();

    Stitch::connect(reader1.sink, writer.source);
    Stitch::connect(reader2.sink, writer.source);

### Stream producers and consumers

Stitch has a couple handy subclasses of Server and Client to model connections between stream producers and consumers:

- [Stream_Producer](@ref Stitch::Stream_Producer)
- [Stream_Consumer](@ref Stitch::Stream_Consumer)

Stream_Consumer is a Server and Stream_Producer is a Client and they share a MPSC_Queue.

This implements the flow of data in the [Actor model][]: the queue represents the mailbox of the receiver of messages (consumer) and messages can arrive from multiple senders (producers).

Example:

    Stream_Producer<int> producer;
    Stream_Consumer<int> consumer(5);

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

[Actor model]: https://en.wikipedia.org/wiki/Actor_model
