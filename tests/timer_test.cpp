#include "../linux/timer.h"

#include <thread>
#include <chrono>
#include <iostream>

using namespace std;
using namespace Reactive;

string time_since(const chrono::steady_clock::time_point & start)
{
    using namespace chrono;

    auto d = chrono::steady_clock::now() - start;
    double s = duration_cast<duration<double>>(d).count();
    return to_string(s);
}

void test_single_shot()
{
    cout << endl << "Single shot" << endl;

    Timer t;
    t.setInterval(chrono::milliseconds(1250), false);

    auto start = chrono::steady_clock::now();

    t.wait();

    cout << time_since(start) << endl;
}

void test_repeated()
{
    cout << endl << "Repeated" << endl;

    Timer t;
    t.setInterval(chrono::milliseconds(250), true);

    auto start = chrono::steady_clock::now();

    for (int i = 0; i < 3; ++i)
    {
        t.wait();

        cout << time_since(start) << endl;
    }
}

void test_subscribe()
{
    cout << endl << "Subscribe" << endl;

    Timer t1;
    t1.setInterval(chrono::milliseconds(250), false);

    Timer t2;
    t2.setInterval(chrono::milliseconds(250), true);

    Event_Reactor r;

    auto start = chrono::steady_clock::now();

    t1.subscribe(r, [&](){
        cout << time_since(start) << " One" << endl;
    });

    int reps = 3;

    t2.subscribe(r, [&](){
        cout << time_since(start) << " Two" << endl;

        if (--reps == 0)
            r.quit();
    });

    r.run(Event_Reactor::WaitUntilQuit);
}

int main()
{
    test_single_shot();

    test_repeated();

    test_subscribe();
}
