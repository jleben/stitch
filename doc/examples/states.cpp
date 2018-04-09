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
