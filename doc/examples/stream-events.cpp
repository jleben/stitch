
struct Notification { int value; };

// Create producer of notifications
Stitch::Stream_Producer<Notification> producer;

thread producer_thread([&]()
{
    // Produce 10 notifications, one every 100 ms
    for (int v = 0; v < 10; ++v)
    {
        producer.push(Notification{v});

        this_thread::sleep_for(chrono::milliseconds(100));
    }
});

// Define consumer of notifications
auto consumer_func = [&]()
{
    // Create a stream consumer that allows buffering up to 100 items.
    Stitch::Stream_Consumer<Notification> consumer(100);
    connect(producer, consumer);

    // Consume notifications until at least 10 are consumed
    int count = 0;
    while(count < 10)
    {
        wait(consumer.event());

        Notification notification;
        while(consumer.pop(notification))
        {
            ++count;
            cout << notification.value << endl;
        }
    }
};

// Start 2 consumers
thread consumer1_thread(consumer_func);
thread consumer2_thread(consumer_func);
