#include "../linux/stream.h"
#include "../testing/testing.h"

using namespace Reactive;
using namespace Testing;
using namespace std;

bool test_basic()
{
    Test test;

    Stream_Source<int> source;
    Stream_Sink<int> sink;

    test.assert("Sink is empty.", sink.empty());

    connect(source, sink);

    test.assert("Connected.", are_connected(source, sink));
    test.assert("Source has connections.", source.has_connections());
    test.assert("Sink has connections.", sink.has_connections());

    for (int i = 0; i < 5; ++i)
    {
        source.push(i);
    }

    for (int i = 0; i < 5; ++i)
    {
        int v = sink.pop();
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

bool test_capacity()
{
    Test test;

    Stream_Source<int> source;
    Stream_Sink<int> sink(5);

    connect(source, sink);

    for (int i = 0; i < 5; ++i)
    {
        source.push(i);
    }

    bool failed_to_push = false;

    try { source.push(9); }
    catch (std::exception &) { failed_to_push = true; }

    test.assert("Failed to push when full.", failed_to_push);

    return test.success();
}

bool test_one_to_many()
{
    Test test;

    Stream_Source<int> source;
    Stream_Sink<int> sink1;
    Stream_Sink<int> sink2;

    connect(source, sink1);
    connect(source, sink2);

    for (int i = 0; i < 5; ++i)
    {
        source.push(i);
    }

    for (int i = 0; i < 5; ++i)
    {
        int v = sink1.pop();
        test.assert("Sink 1 received: " + to_string(v), v == i);
    }

    test.assert("Sink 1 is empty.", sink1.empty());

    for (int i = 0; i < 5; ++i)
    {
        int v = sink2.pop();
        test.assert("Sink 2 received: " + to_string(v), v == i);
    }

    test.assert("Sink 2 is empty.", sink2.empty());

    return test.success();
}

bool test_many_to_one()
{
    Test test;

    Stream_Source<int> source1;
    Stream_Source<int> source2;
    Stream_Sink<int> sink;

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
        v = sink.pop();
        test.assert("Sink received: " + to_string(v), v == i);
        v = sink.pop();
        test.assert("Sink received: " + to_string(v), v == i * 10);
    }

    test.assert("Sink is empty.", sink.empty());

    return test.success();
}

bool test_disconnect_when_destroyed()
{
    Test test;

    {
        Stream_Source<int> source;
        {
            Stream_Sink<int> sink;
            connect(source, sink);
        }
        test.assert("Source has no connections.", !source.has_connections());
    }
    {
        Stream_Sink<int> sink;
        {
            Stream_Source<int> source;
            connect(source, sink);
        }
        test.assert("Sink has no connections.", !sink.has_connections());
    }

    return test.success();
}

int main(int argc, char * argv[])
{
    Test_Set s = {
        { "basic", test_basic },
        { "capacity", test_capacity },
        { "one to many", test_one_to_many },
        { "many to one", test_many_to_one },
        { "disconnect when destroyed", test_disconnect_when_destroyed },
    };

    return run(s, argc, argv);
}
