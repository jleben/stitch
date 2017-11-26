#include "../concurrency/timer.h"
#include "../testing/testing.h"

#include <thread>
#include <chrono>
#include <iostream>

using namespace std;
using namespace Stitch;
using namespace Testing;

static bool test_single_shot()
{
    Timer t;
    t.start(chrono::milliseconds(1250), false);

    auto start = chrono::steady_clock::now();

    t.wait();

    cout << time_since(start) << endl;

    return true;
}

static bool test_repeated()
{
    Test test;

    Timer t;
    t.start(chrono::milliseconds(250), true);

    auto start = chrono::steady_clock::now();

    for (int i = 0; i < 3; ++i)
    {
        t.wait();

        cout << time_since(start) << endl;
    }

    test.assert("Completed.", true);

    return test.success();
}

static bool test_subscribe()
{
    Test t;

    Timer t1;
    t1.start(chrono::milliseconds(250), false);

    Timer t2;
    t2.start(chrono::milliseconds(250), true);

    Event_Reactor r;

    auto start = chrono::steady_clock::now();

    int one_count = 0;

    r.subscribe(t1.event(), [&](){
        ++one_count;
        cout << time_since(start) << " One" << endl;
    });

    int reps = 3;
    int two_count = 0;

    r.subscribe(t2.event(), [&](){
        ++two_count;
        cout << time_since(start) << " Two" << endl;

        if (two_count == reps)
            r.quit();
    });

    r.run(Event_Reactor::WaitUntilQuit);

    t.assert("Correct event count.", one_count == 1 && two_count == reps);

    return t.success();
}

static bool test_restart()
{
    Test test;

    Timer t;

    auto start = chrono::steady_clock::now();

    t.start(chrono::milliseconds(500), false);

    this_thread::sleep_for(chrono::milliseconds(250));

    t.start(chrono::milliseconds(500), false);

    t.wait();

    double e = time_since(start);

    cout << e << endl;

    test.assert("Elapsed time >= 0.75.", e >= 0.75);

    return test.success();
}

static bool test_stop()
{
    Test test;

    Timer t1;
    Timer t2;

    auto start = chrono::steady_clock::now();

    t1.start(chrono::milliseconds(250), false);
    t2.start(chrono::milliseconds(500), false);
    t1.stop();

    Event_Reactor r;

    r.subscribe(t1.event(), [&](){
        cout << time_since(start) << endl;
        cout << "Not OK" << endl;
    });

    r.subscribe(t2.event(), [&](){
        cout << time_since(start) << endl;
        cout << "OK" << endl;
    });

    r.run(Event_Reactor::Wait);

    return test.success();
}

Test_Set timer_tests()
{
    return {
        { "Single Shot", test_single_shot },
        { "Repeated", test_repeated },
        { "Subscribe", test_subscribe },
        { "Restart", test_restart },
        { "Stop", test_stop },
    };
}
