#include "../stitch/signal.h"
#include "../testing/testing.h"

#include <thread>
#include <iostream>

using namespace Stitch;
using namespace std;
using namespace Testing;

static bool test_wait()
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

static bool test_wait_multi()
{
    Test test;

    Signal s;

    thread t1([&]()
    {
        s.wait();
        test.assert("Thread one notified.", true);
    });

    thread t2([&]()
    {
        s.wait();
        test.assert("Thread two notified.", true);
    });

    // Wait to make sure both threads are waiting.
    this_thread::sleep_for(chrono::milliseconds(100));

    s.notify();

    t1.join();
    t2.join();

    return test.success();
}

static bool test_send_one_to_many()
{
    Test test;

    Signal_Sender s;
    Signal_Receiver r1;
    Signal_Receiver r2;

    connect(s, r1);
    connect(s, r2);

    atomic<int> index { 0 };

    s.notify();

    auto receiver = ([&](int id)
    {
        Signal_Receiver & r = id == 1 ? r1 : r2;

        r.wait();
        test.assert("Receiver notified.", index == 0);

        auto start = chrono::steady_clock::now();
        r.wait();
        auto end = chrono::steady_clock::now();

        test.assert("Receiver waited.",
                    end - start > chrono::milliseconds(80) &&
                    end - start < chrono::milliseconds(120));
        test.assert("Receiver woken up at the right time.", index == 1);
    });

    thread t1(receiver, 1);
    thread t2(receiver, 2);

    this_thread::sleep_for(chrono::milliseconds(100));

    index = 1;

    s.notify();

    t1.join();
    t2.join();

    return test.success();
}

static bool test_send_many_to_one()
{
    Test test;

    Signal_Sender s1;
    Signal_Sender s2;
    Signal_Receiver r;

    connect(s1, r);
    connect(s2, r);

    atomic<int> index { 0 };

    s1.notify();
    s2.notify();

    thread t([&]()
    {
        r.wait();
        test.assert("Receiver woken immediately.", index == 0);

        auto start = chrono::steady_clock::now();
        r.wait();
        auto end = chrono::steady_clock::now();

        test.assert("Receiver waited.",
                    end - start > chrono::milliseconds(80) &&
                    end - start < chrono::milliseconds(120));
        test.assert("Receiver woken up at the right time.", index == 1);

        r.wait();
        end = chrono::steady_clock::now();

        test.assert("Receiver waited.",
                    end - start > chrono::milliseconds(180) &&
                    end - start < chrono::milliseconds(220));
        test.assert("Receiver woken up at the right time.", index == 2);
    });

    this_thread::sleep_for(chrono::milliseconds(100));

    index = 1;

    s1.notify();

    this_thread::sleep_for(chrono::milliseconds(100));

    index = 2;

    s2.notify();

    t.join();

    return test.success();
}

static bool test_subscribe()
{
    Test test;

    Event_Reactor reactor;

    Signal signal;
    int count = 0;
    reactor.subscribe(signal.event(), [&](){ ++count; });

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

Test_Set signal_tests()
{
    return {
        { "wait", test_wait },
        { "wait-multi", test_wait_multi },
        { "send-one-to-many", test_send_one_to_many },
        { "send-many-to-one", test_send_many_to_one },
        { "subscribe", test_subscribe },
    };
}
