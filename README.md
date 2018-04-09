
Stitch is a C++ library for communication and synchronization of threads and the outside world, suitable for real-time applications.

See the [documentation](http://webhome.csc.uvic.ca/~jleben/stitch/) for details.

See the [comparison with related software](#comparison-with-related-software) below.

## Principles

The design of Stitch is guided by the following principles:

### Modularity

Instead of implementing a particular communication model (e.g. Actors, Communicating Sequential Processes, ...), Stitch provides basic building blocks. The blocks are easy to compose, and the user is free to compose them in the way most suitable for their application.

However, Stitch does provide implementations of several common [communication patterns](#patterns) built on top of the basic blocks.

### Separation of Concerns

Stitch separates two concerns:

1. Information: Producing and consuming data; addressed by [shared data structures](#shared-data-structures).
2. Time: Waiting for something to happen, for example data to be produced and consumed; addressed by the [event system](#events).

Despite striving to separate these concerns, Stitch makes sure the classes that represent them are easy to compose. This is demonstrated by Stitch's higher-level classes implementing common [communication patterns](#patterns).

### Real-time safety

Stitch aims to be suitable for real-time applications. Its goal is to provide at least lock-free progress guarantees and linear or constant time complexity for as many operations as possible. Progress guarantees and complexity should be documented for each individual operation (work in progress).


## Shared Data Structures

Stitch provides a number of wait-free and lock-free data structures. Here's an example of a queue and an atom:

    struct Data { int x, y, z; }

    Stitch::Atom<Data> atom;
    Stitch::SPSC_Queue<int> queue;

    thread t1([&]()
    {
        Stitch::AtomWriter atom_writer(atom);
        atom_writer.store({ 1, 2, 3 }); // Lock-free
        queue.push(123); // Wait-free
    });

    thread t2([&]()
    {
        Stitch::AtomReader atom_reader(atom);
        Data a = atom_reader.load(); // Lock-free
        int b;
        bool ok = queue.pop(b); // Wait-free
    });

## Events

Stitch provides an event system allowing to wait for multiple events of different types.

    Stitch::Timer timer;
    Stitch::Signal signal;

    timer.start(chrono::milliseconds(1000));

    thread notifier([&]()
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

    reactor.run(Stitch::Event_Reactor::WaitUntilQuit);

## Patterns

Building upon shared data structures and events, Stitch provides classes that implement common communication patterns.

### Stream producers and consumers

Stream producers and consumers communicate streams of items. They can be connected in a many-to-many fashion.

    struct Work { int input; };

    // Create a stream consumer that allows 100 work items to be buffered.
    Stitch::Stream_Consumer<Work> consumer(100);

    thread consumer_thread([&]()
    {
        // Consume items until at least 10 are consumed
        int count = 0;
        while(count < 10)
        {
            wait(consumer.event())

            Work item;
            while(consumer.pop(item))
            {
                ++count
                cout << item.input << endl;
            }
        }
    });

    // Define a producer of work
    auto producer_func = [&]()
    {
        Stitch::Stream_Producer<Work> producer;
        connect(producer, consumer);

        // Produce 10 work requests, one every 100 ms
        for (int v = 0; v < 10; ++v)
        {
            producer.push({v});

            this_thread::sleep_for(chrono::milliseconds(100));
        }
    };

    // Start 2 producers
    thread producer1_thread(producer_func);
    thread producer2_thread(producer_func);

### State and Observers

State updater and observers communicate the latest state of a value.

    Stitch::State<int> state;

    thread writer_thread([&]()
    {
        // Update state 10 times, every 100 ms
        for (int v = 1; v <= 10; ++v)
        {
            state.store(v);

            this_thread::sleep_for(chrono::milliseconds(100));
        }
    });

    auto observer_func = [&]()
    {
        Stitch::State_Observer<int> observer;
        observer.connect(state);

        // React to state changes, until state equals 10
        int v;
        do {
            wait(observer.changed());

            v = observer.load();
            cout << v << endl;
        }
        while(v != 10);
    };

    // Start 2 observers
    thread observer1_thread(observer_func);
    thread observer2_thread(observer_func);















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
