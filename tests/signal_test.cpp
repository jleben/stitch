#include "../linux/signal.h"
#include "../testing/testing.h"

#include <thread>
#include <iostream>

using namespace Reactive;
using namespace std;
using namespace Testing;

bool test_wait()
{
    Test test;

    Signal s;

    auto e = s.event();

    thread t1([&](){

        s.notify();

        this_thread::sleep_for(chrono::milliseconds(200));

        s.notify();
    });

    double elapsed = 0;
    auto start = chrono::steady_clock::now();

    s.wait();

    elapsed = Testing::time_since(start);
    test.assert("Elapsed time ~= 0", elapsed < 0.02);

    s.wait();

    elapsed = Testing::time_since(start);
    test.assert("Elapsed time ~= 0.2", elapsed > 0.18 && elapsed < 0.22);

    t1.join();

    return test.success();
}

bool test_stream()
{
    Test test;

    Event_Reactor reactor;

    Signal signal;
    int count = 0;
    signal.stream(reactor).subscribe([&](){ ++count; });

    thread t1([&]()
    {
        int count = 5;
        while(count--)
        {
            signal.notify();
            this_thread::sleep_for(chrono::milliseconds(50));
        }
    });

    while(count < 5)
        reactor.run(Event_Reactor::Wait);

    t1.join();

    test.assert("Event count = " + to_string(count), count == 5);

    return test.success();
}

int main(int argc, char * argv[])
{
    Test_Set t = {
        { "wait", test_wait },
        { "stream", test_stream },
    };

    return Testing::run(t, argc, argv);
}

