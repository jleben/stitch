#include "../signal.h"
#include "../timer.h"
#include "../events.h"

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

    Waiter w({ e1, e2, e3 });

    for(;;)
    {
        auto e = w.wait();

        if (e == e1)
        {
            cout << "One." << endl;
        }
        else if (e == e2)
        {
            cout << "Two." << endl;
        }
        else if (e == e3)
        {
            cout << "Three." << endl;
        }
        else
        {
            cout << "Unknown." << endl;
        }
    }
}
