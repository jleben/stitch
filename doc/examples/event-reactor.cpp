// Create a periodic timer with a period of 100 ms

Stitch::Timer timer;
timer.start(chrono::milliseconds(100), true);

// Start a thread generating an event every 200 ms,
// each time incrementing a counter.

Stitch::Signal signal;
atomic<int> counter;
thread t1([&]()
{
    for(;;)
    {
        this_thread::sleep_for(chrono::milliseconds(200));
        counter.fetch_add(1);
        signal.notify();
    }
});

// Create an event reactor and subscribe it
// to the timer and signal events.

Stitch::Event_Reactor reactor;

reactor.subscribe(timer.event(), [&]()
{
    cout << "Timer" << endl;
});

reactor.subscribe(signal.event(), [&]()
{
    cout << "Signal" << endl;

    // Tell reactor to quit waiting for events
    // when the counter reaches 10.

    if (counter >= 10)
        reactor.quit();
});

// Start waiting and reacting to events until
// reactor is told to quit.

reactor.run(Stitch::Event_Reactor::WaitUntilQuit);
