#include "../interface/signal.h"
#include "../interface/timer.h"
#include "../interface/events.h"

#include <thread>
#include <chrono>
#include <iostream>

using namespace Concurrency;
using namespace std;
using namespace std::chrono;

string time_since(const chrono::steady_clock::time_point & start)
{
    auto d = chrono::steady_clock::now() - start;
    double s = duration_cast<duration<double>>(d).count();
    return to_string(s);
}

int main()
{
    Signal s1;
    Signal s2;

    thread t1([&]()
    {
        for(;;)
        {
            this_thread::sleep_for(chrono::milliseconds(1000));

            s1.notify();
        }
    });

    thread t2([&]()
    {
        for(;;)
        {
            this_thread::sleep_for(chrono::milliseconds(2000));

            s2.notify();
        }
    });

    Timer t;
    t.setInterval(3, true);

    auto e1 = s1.event();
    auto e2 = s2.event();
    auto e3 = t.event();

    Waiter w;
    w.add_event(e1);
    w.add_event(e2);
    w.add_event(e3);

    auto start = chrono::steady_clock::now();

    for(;;)
    {
        w.wait();

        if (*e1)
        {
            cout << time_since(start) << " One." << endl;
            e1->clear();
        }
        if (*e2)
        {
            cout << time_since(start) << " Two." << endl;
            e2->clear();
        }
        if (*e3)
        {
            cout << time_since(start) << " Three." << endl;
            e3->clear();
        }
    }
}
