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

    auto start = chrono::steady_clock::now();

    e2->wait();

    cout << time_since(start) << " Initial Two." << endl;

    e3->wait();

    cout << time_since(start) << " Initial Three." << endl;

    Waiter w;
    e1->subscribe(&w, [&](){
        cout << time_since(start) << " One." << endl;
    });
    e2->subscribe(&w, [&](){
        cout << time_since(start) << " Two." << endl;
    });
    e3->subscribe(&w, [&](){
        cout << time_since(start) << " Three." << endl;
    });

    for(;;)
    {
        w.wait();
    }
}
