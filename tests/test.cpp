#include "../interface/signal.h"
#include "../interface/timer.h"
#include "../interface/events.h"

#include <thread>
#include <chrono>
#include <iostream>

using namespace Concurrency;
using namespace std;
using namespace std::chrono;

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

    for(;;)
    {
        w.wait();

        if (*e1)
        {
            cout << "One." << endl;
            e1->clear();
        }
        else if (*e2)
        {
            cout << "Two." << endl;
            e2->clear();
        }
        else if (*e3)
        {
            cout << "Three." << endl;
            e3->clear();
        }
        else
        {
            cout << "Unknown." << endl;
        }
    }
}
