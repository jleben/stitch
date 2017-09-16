#include "../linux/timer.h"

#include <thread>
#include <chrono>
#include <iostream>

using namespace std;
using namespace Reactive;

double time_since(const chrono::steady_clock::time_point & start)
{
    using namespace chrono;

    auto d = chrono::steady_clock::now() - start;
    double s = duration_cast<duration<double>>(d).count();
    return s;
}

void test_single_shot()
{
    cout << endl << "Single shot" << endl;

    Timer t;
    t.start(chrono::milliseconds(1250), false);

    auto start = chrono::steady_clock::now();

    t.wait();

    cout << time_since(start) << endl;

    cout << "OK" << endl;
}

void test_repeated()
{
    cout << endl << "Repeated" << endl;

    Timer t;
    t.start(chrono::milliseconds(250), true);

    auto start = chrono::steady_clock::now();

    for (int i = 0; i < 3; ++i)
    {
        t.wait();

        cout << time_since(start) << endl;
    }

    cout << "OK" << endl;
}

void test_subscribe()
{
    cout << endl << "Subscribe" << endl;

    Timer t1;
    t1.start(chrono::milliseconds(250), false);

    Timer t2;
    t2.start(chrono::milliseconds(250), true);

    Event_Reactor r;

    auto start = chrono::steady_clock::now();

    int one_count = 0;

    t1.subscribe(r, [&](){
        ++one_count;
        cout << time_since(start) << " One" << endl;
    });

    int reps = 3;
    int two_count = 0;

    t2.subscribe(r, [&](){
        ++two_count;
        cout << time_since(start) << " Two" << endl;

        if (two_count == reps)
            r.quit();
    });

    r.run(Event_Reactor::WaitUntilQuit);

    if (one_count == 1 && two_count == reps)
        cout << "OK" << endl;
    else
        cout << "Not OK" << endl;
}

void test_restart()
{
    cout << endl << "Restart" << endl;

    Timer t;

    auto start = chrono::steady_clock::now();

    t.start(chrono::milliseconds(500), false);

    this_thread::sleep_for(chrono::milliseconds(250));

    t.start(chrono::milliseconds(500), false);

    t.wait();

    double e = time_since(start);

    cout << e << endl;

    if (e >= 0.75)
        cout << "OK" << endl;
    else
        cout << "Not OK" << endl;
}

void test_stop()
{
    cout << endl << "Stop" << endl;

    Timer t1;
    Timer t2;

    auto start = chrono::steady_clock::now();

    t1.start(chrono::milliseconds(250), false);
    t2.start(chrono::milliseconds(500), false);
    t1.stop();

    Event_Reactor r;

    t1.subscribe(r, [&](){
        cout << time_since(start) << endl;
        cout << "Not OK" << endl;
    });

    t2.subscribe(r, [&](){
        cout << time_since(start) << endl;
        cout << "OK" << endl;
    });

    r.run(Event_Reactor::Wait);
}

int main()
{
    test_single_shot();

    test_repeated();

    test_subscribe();

    test_restart();

    test_stop();
}
