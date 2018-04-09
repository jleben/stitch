
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
