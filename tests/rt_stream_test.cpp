#include "../linux/rt_stream.h"
#include "utils.h"

#include <thread>
#include <sstream>

using namespace Reactive;
using namespace Testing;
using namespace std;

bool test_basic()
{
    Testing::Test test;

    Realtime_Stream_Source<int> source;
    Realtime_Stream_Sink<int> sink;

    connect(source, sink, 15);

    test.assert("Source is connected.", source.is_connected());
    test.assert("Sink is connected.", sink.is_connected());

    vector<int> received;

    for (int i = 0; i < 10; ++i)
    {
        source.push(i);
    }

    for (int i = 0; i < 10; ++i)
    {
        int val = sink.pop();
        received.push_back(val);
    }

    for (int i = 10; i < 20; ++i)
    {
        source.push(i);
    }

    for (int i = 10; i < 20; ++i)
    {
        int val = sink.pop();
        received.push_back(val);
    }

    for(int i = 0; i < received.size(); ++i)
    {
        test.assert("received[" + to_string(i) + "] = " + to_string(i), received[i] == i);
    }

    disconnect(source, sink);

    test.assert("Source is not connected.", !source.is_connected());
    test.assert("Sink is not connected.", !sink.is_connected());

    return test.success();
}

bool test_iterators()
{
    Testing::Test test;

    Realtime_Stream_Source<int> source;
    Realtime_Stream_Sink<int> sink;

    connect(source, sink, 10);

    vector<int> received;

    int w = 0;

    for (int i = 0; i < 5; ++i)
    {
        source.push(w);
        ++w;
    }

    for (int i = 0; i < 5; ++i)
    {
        int val = sink.pop();
        received.push_back(val);
    }

    for (auto & available : source.range(3))
    {
        available = w;
        ++w;
    }

    for (auto & available : sink.range(3))
    {
        received.push_back(available);
    }

    for (auto & available : source.range())
    {
        available = w;
        ++w;
    }

    for (auto & available : sink.range())
    {
        received.push_back(available);
    }

    test.assert("Received count = 17", received.size() == 17);

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
            Realtime_Stream_Sink<int> sink;
            connect(source, sink, 1);
        }
        test.assert("Source is not connected.", !source.is_connected());
    }
    {
        Realtime_Stream_Sink<int> sink;
        {
            Realtime_Stream_Source<int> source;
            connect(source, sink, 1);
        }
        test.assert("Sink is not connected.", !sink.is_connected());
    }

    return test.success();
}

int main(int argc, char * argv[])
{
    Test_Set t = {
        { "basic", test_basic },
        { "iterators", test_iterators },
        { "disconnect when destroyed ", test_disconnect_when_destroyed }
    };

    return Testing::run(t, argc, argv);
}
