#include "../linux/rt_stream.h"
#include "utils.h"

#include <thread>
#include <sstream>

using namespace Reactive;
using namespace std;

bool test_basic()
{
    Testing::Test test;

    Realtime_Stream_Source<int> source;
    Realtime_Stream_Sink<int> sink;

    connect(source, sink, 15);

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

int main(int argc, char * argv[])
{
    Testing::Test_Set t =
    { { "basic", test_basic },
      { "iterators", test_iterators } };

    return Testing::run(t, argc, argv);
}
