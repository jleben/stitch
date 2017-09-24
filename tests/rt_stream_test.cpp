#include "../linux/rt_stream.h"
#include "../testing/testing.h"

#include <thread>
#include <sstream>

using namespace Reactive;
using namespace Testing;
using namespace std;

bool test_basic()
{
    Testing::Test test;

    Realtime_Stream_Source<int> source;
    Realtime_Stream_Sink<int> sink(15);

    test.assert("Sink capacity is 15.", sink.capacity() == 15);

    connect(source, sink);

    test.assert("Source is connected.", source.is_connected());
    test.assert("Sink is connected.", sink.is_connected());

    vector<int> received;

    for (int i = 0; i < 10; ++i)
    {
        source.push(i);
    }

    test.assert("Sink count = 10.", sink.count() == 10);

    for (int i = 0; i < 10; ++i)
    {
        int val = sink.pop();
        received.push_back(val);
    }

    test.assert("Sink is empty.", sink.count() == 0);

    for (int i = 10; i < 20; ++i)
    {
        source.push(i);
    }

    test.assert("Sink count = 10.", sink.count() == 10);

    for (int i = 10; i < 20; ++i)
    {
        int val = sink.pop();
        received.push_back(val);
    }

    test.assert("Sink is empty.", sink.count() == 0);

    for(int i = 0; i < received.size(); ++i)
    {
        test.assert("received[" + to_string(i) + "] = " + to_string(i), received[i] == i);
    }

    disconnect(source, sink);

    test.assert("Source is not connected.", !source.is_connected());
    test.assert("Sink is not connected.", !sink.is_connected());

    connect(source, sink);

    test.assert("Source is reconnected.", source.is_connected());
    test.assert("Sink is reconnected.", sink.is_connected());

    for (int i = 0; i < 3; ++i)
    {
        source.push(i);
    }

    test.assert("Sink count = 3.", sink.count() == 3);

    for (int i = 0; i < 3; ++i)
    {
        int val = sink.pop();
        test.assert("Received " + to_string(val), val == i);
    }

    test.assert("Sink is empty.", sink.count() == 0);

    return test.success();
}

bool test_iterators()
{
    Testing::Test test;

    Realtime_Stream_Source<int> source;
    Realtime_Stream_Sink<int> sink(10);

    connect(source, sink);

    vector<int> received;

    int w = 0;

    for (int i = 0; i < 5; ++i)
    {
        source.push(w);
        ++w;
    }

    test.assert("Sink count = 5.", sink.count() == 5);

    for (int i = 0; i < 5; ++i)
    {
        int val = sink.pop();
        received.push_back(val);
    }

    test.assert("Received count = 5", received.size() == 5);
    test.assert("Sink is empty.", sink.count() == 0);

    for (auto & available : source.range(3))
    {
        available = w;
        ++w;
    }

    for (auto & available : sink.range(3))
    {
        received.push_back(available);
    }

    test.assert("Received count = 8", received.size() == 8);
    test.assert("Sink is empty.", sink.count() == 0);

    for (auto & available : source.range())
    {
        available = w;
        ++w;
    }

    test.assert("Sink count = 10.", sink.count() == 10);

    for (auto & available : sink.range())
    {
        received.push_back(available);
    }

    test.assert("Received count = 18", received.size() == 18);
    test.assert("Sink is empty.", sink.count() == 0);

    for(int i = 0; i < received.size(); ++i)
    {
        ostringstream msg;
        msg << "received[" << i << "] = " << received[i];
        test.assert(msg.str(), received[i] == i);
    }

    return test.success();
}

bool test_disconnect_when_destroyed()
{
    Test test;

    {
        Realtime_Stream_Source<int> source;
        {
            Realtime_Stream_Sink<int> sink(1);
            connect(source, sink);
        }
        test.assert("Source is not connected.", !source.is_connected());
    }
    {
        Realtime_Stream_Sink<int> sink(1);
        {
            Realtime_Stream_Source<int> source;
            connect(source, sink);
        }
        test.assert("Sink is not connected.", !sink.is_connected());
    }

    return test.success();
}

bool test_disconnected_source()
{
    Test test;

    Realtime_Stream_Source<int> source;

    for (int i = 0; i < 10; ++i)
        source.push(i);

    test.assert("Push does nothing.", true);

    bool range_is_empty = true;
    for (auto & val : source.range())
        range_is_empty = false;

    test.assert("Range is empty.", range_is_empty);

    return test.success();
}

bool test_disconnected_sink()
{
    Test test;

    Realtime_Stream_Sink<int> sink(5);

    test.assert("Capacity is 5.", sink.capacity() == 5);
    test.assert("Count is 0.", sink.count() == 0);

    bool range_is_empty = true;
    for (const auto & val : sink.range())
        range_is_empty = false;
    test.assert("Range is empty.", range_is_empty);

    bool non_empty_range_throws = false;
    try { auto r = sink.range(5); }
    catch (std::exception&) { non_empty_range_throws = true; }
    test.assert("Requesting a non-empty range throws.", non_empty_range_throws);

    return test.success();
}

bool test_full_source()
{
    Test test;

    Realtime_Stream_Source<int> source;
    Realtime_Stream_Sink<int> sink(1);

    connect(source, sink);

    source.push(1);

    bool pushing_throws = false;
    try { source.push(2); }
    catch (std::exception&) { pushing_throws = true; }
    test.assert("Pushing throws.", pushing_throws);

    bool range_is_empty = true;
    for (auto & val : source.range())
        range_is_empty = false;
    test.assert("Range is empty.", range_is_empty);

    bool non_empty_range_throws = false;
    try { auto r = source.range(5); }
    catch (std::exception&) { non_empty_range_throws = true; }
    test.assert("Requesting a non-empty range throws.", non_empty_range_throws);

    return test.success();
}

int main(int argc, char * argv[])
{
    Test_Set t = {
        { "basic", test_basic },
        { "iterators", test_iterators },
        { "disconnect when destroyed ", test_disconnect_when_destroyed },
        { "disconnected source ", test_disconnected_source },
        { "disconnected sink ", test_disconnected_sink },
        { "full source", test_full_source }
    };

    return Testing::run(t, argc, argv);
}
