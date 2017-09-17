#include "../linux/channel.h"
#include "utils.h"

#include <thread>
#include <sstream>

using namespace Reactive;
using namespace std;

void test_basic()
{
    Stream_Producer<int> producer;
    Stream_Consumer<int> consumer;

    connect(producer, consumer, 15);

    vector<int> received;

    for (int i = 0; i < 10; ++i)
    {
        producer.push(i);
    }

    for (int i = 0; i < 10; ++i)
    {
        int val = consumer.pop();
        received.push_back(val);
    }

    for (int i = 10; i < 20; ++i)
    {
        producer.push(i);
    }

    for (int i = 10; i < 20; ++i)
    {
        int val = consumer.pop();
        received.push_back(val);
    }

    for(int i = 0; i < received.size(); ++i)
    {
        Test::assert("received[" + to_string(i) + "] = " + to_string(i), received[i] == i);
    }
}

void test_iterators()
{
    Stream_Producer<int> producer;
    Stream_Consumer<int> consumer;

    connect(producer, consumer, 10);

    vector<int> received;

    int w = 0;

    for (int i = 0; i < 5; ++i)
    {
        producer.push(w);
        ++w;
    }

    for (int i = 0; i < 5; ++i)
    {
        int val = consumer.pop();
        received.push_back(val);
    }

    for (auto & available : producer.range(3))
    {
        available = w;
        ++w;
    }

    for (auto & available : consumer.range(3))
    {
        received.push_back(available);
    }

    for (auto & available : producer.range())
    {
        available = w;
        ++w;
    }

    for (auto & available : consumer.range())
    {
        received.push_back(available);
    }

    Test::assert("Received count = 17", received.size() == 17);

    for(int i = 0; i < received.size(); ++i)
    {
        ostringstream msg;
        msg << "received[" << i << "] = " << received[i];
        Test::assert(msg.str(), received[i] == i);
    }
}

int main()
{
    test_basic();

    test_iterators();
}
