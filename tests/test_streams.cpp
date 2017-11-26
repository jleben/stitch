#include "../stitch/streams.h"
#include "../testing/testing.h"

using namespace Stitch;
using namespace Testing;
using namespace std;


static bool test_push_unconnected()
{
    Stream_Producer<int> p;

    for(int i = 0; i < 10; ++i)
        p.push(i);

    return true;
}

static bool test_pop_unconnected()
{
    Test test;

    Stream_Consumer<int> c(5);

    for(int i = 0; i < 10; ++i)
    {
        int v;
        bool ok = c.pop(v);
        test.assert("Pop prevented.", !ok);
    }

    return test.success();
}

static bool test_connection()
{
    Test test;

    // Explicitly connect and disconnect

    {
        Stream_Producer<int> source;
        Stream_Consumer<int> sink(1);

        connect(source, sink);
        test.assert("Connected.", are_connected(source, sink));
        test.assert("Source has connections.", source.has_connections());
        test.assert("Sink has connections.", sink.has_connections());

        disconnect(source, sink);
        test.assert("Disconnected.", !are_connected(source, sink));
        test.assert("Source has no connections.", !source.has_connections());
        test.assert("Sink has no connections.", !sink.has_connections());
    }

    // Auto disconnect when destroyed

    {
        Stream_Producer<int> source;
        {
            Stream_Consumer<int> sink(1);
            connect(source, sink);
            test.assert("Connected.", are_connected(source, sink));
        }
        test.assert("Source has no connections.", !source.has_connections());
    }
    {
        Stream_Consumer<int> sink(1);
        {
            Stream_Producer<int> source;
            connect(source, sink);
            test.assert("Connected.", are_connected(source, sink));
        }
        test.assert("Sink has no connections.", !sink.has_connections());
    }

    return test.success();
}

static bool test_basic()
{
    Test test;

    Stream_Producer<int> source;
    Stream_Consumer<int> sink(5);

    test.assert("Sink is empty.", sink.empty());

    connect(source, sink);

    test.assert("Connected.", are_connected(source, sink));
    test.assert("Source has connections.", source.has_connections());
    test.assert("Sink has connections.", sink.has_connections());

    for (int i = 0; i < 5; ++i)
    {
        source.push(i);
    }

    test.assert("Sink is not empty.", !sink.empty());

    for (int i = 0; i < 5; ++i)
    {
        int v;
        bool ok = sink.pop(v);
        test.assert("Received.", ok);
        if (ok)
            test.assert("Received: " + to_string(v), v == i);
    }

    test.assert("Sink is empty.", sink.empty());

    disconnect(source, sink);

    test.assert("Disconnected.", !are_connected(source, sink));
    test.assert("Source has no connections.", !source.has_connections());
    test.assert("Sink has no connections.", !sink.has_connections());

    source.push(312);

    test.assert("Sink is still empty.", sink.empty());

    return test.success();
}

static bool test_exceeding_capacity()
{
    Test test;

    Stream_Producer<int> source;
    Stream_Consumer<int> sink(5);

    connect(source, sink);

    for (int i = 0; i < 15; ++i)
    {
        source.push(i);
    }

    for (int i = 0; i < 5; ++i)
    {
        int v;
        bool ok = sink.pop(v);
        test.assert("Received.", ok);
        if (ok)
            test.assert("Received: " + to_string(v), v == i);
    }

    return test.success();
}

static bool test_one_to_many()
{
    Test test;

    Stream_Producer<int> source;
    Stream_Consumer<int> sink1(5);
    Stream_Consumer<int> sink2(5);

    connect(source, sink1);
    connect(source, sink2);

    for (int i = 0; i < 5; ++i)
    {
        source.push(i);
    }

    for (int i = 0; i < 5; ++i)
    {
        int v;
        bool ok = sink1.pop(v);
        test.assert("Sink 1 received.", ok);
        if (ok)
            test.assert("Sink 1 received: " + to_string(v), v == i);
    }

    test.assert("Sink 1 is empty.", sink1.empty());

    for (int i = 0; i < 5; ++i)
    {
        int v;
        bool ok = sink2.pop(v);
        test.assert("Sink 2 received.", ok);
        if (ok)
            test.assert("Sink 2 received: " + to_string(v), v == i);
    }

    test.assert("Sink 2 is empty.", sink2.empty());

    return test.success();
}

static bool test_many_to_one()
{
    Test test;

    Stream_Producer<int> source1;
    Stream_Producer<int> source2;
    Stream_Consumer<int> sink(10);

    connect(source1, sink);
    connect(source2, sink);

    for (int i = 0; i < 5; ++i)
    {
        source1.push(i);
        source2.push(i * 10);
    }

    for (int i = 0; i < 5; ++i)
    {
        int v;
        bool ok;

        ok = sink.pop(v);
        test.assert("Sink received.", ok);
        if (ok)
            test.assert("Sink received: " + to_string(v), v == i);

        ok = sink.pop(v);
        test.assert("Sink received.", ok);
        if (ok)
            test.assert("Sink received: " + to_string(v), v == i * 10);
    }

    test.assert("Sink is empty.", sink.empty());

    return test.success();
}

static bool test_bulk()
{
    Test test;

    Stream_Producer<int> source;
    Stream_Consumer<int> sink1(10);
    Stream_Consumer<int> sink2(10);

    connect(source, sink1);
    connect(source, sink2);

    int count = 10;

    {
        vector<int> data(count);
        for(int i = 0; i < count; ++i)
            data[i] = i;
        source.push(count, data.begin());
    }

    {
        vector<int> data(count);

        bool popped = sink1.pop(count, data.begin());
        test.assert("Sink 1 popped.", popped);

        for (int i = 0; i < count; ++i)
            test.assert("Sink 2 got " + to_string(data[i]), data[i] == i);
    }

    {
        vector<int> data(count);

        bool popped = sink2.pop(count, data.begin());
        test.assert("Sink 2 popped.", popped);

        for (int i = 0; i < count; ++i)
            test.assert("Sink 2 got " + to_string(data[i]), data[i] == i);
    }

    return test.success();
}

Test_Set stream_tests()
{
    return {
        { "push unconnected", test_push_unconnected },
        { "pop unconnected", test_pop_unconnected },
        { "connection", test_connection },
        { "basic", test_basic },
        { "exceeding capacity", test_exceeding_capacity },
        { "one to many", test_one_to_many },
        { "many to one", test_many_to_one },
        { "bulk", test_bulk },
    };
}
