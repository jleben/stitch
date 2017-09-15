#include "../interface/signal.h"
#include "../interface/timer.h"
#include "../interface/events.h"

#include <thread>
#include <chrono>
#include <iostream>
#include <atomic>

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
    atomic<bool> stop_threads;

    Signal s1;
    Signal s2;

    thread t1([&]()
    {
        while(!stop_threads)
        {
            this_thread::sleep_for(chrono::milliseconds(1000));

            s1.notify();
        }
    });

    thread t2([&]()
    {
        while(!stop_threads)
        {
            this_thread::sleep_for(chrono::milliseconds(2000));

            s2.notify();
        }
    });

    Timer tm;
    tm.setInterval(3, true);

    Timer q;
    q.setInterval(7);

    auto e1 = s1.event();
    auto e2 = s2.event();

    auto start = chrono::steady_clock::now();

    e1->wait();

    cout << time_since(start) << " !One." << endl;

    e2->wait();

    cout << time_since(start) << " !Two." << endl;

    e1->wait();

    cout << time_since(start) << " !One." << endl;


    Event_Reactor r;
    e1->subscribe(r, [&](){
        cout << time_since(start) << " One." << endl;
    });
    e2->subscribe(r, [&](){
        cout << time_since(start) << " Two." << endl;
    });
    tm.event()->subscribe(r, [&](){
        cout << time_since(start) << " Three." << endl;
    });
    q.event()->subscribe(r, [&](){
        cout << "Quit." << endl;
        r.quit();
    });

    r.run(Event_Reactor::WaitUntilQuit);
#if 0
    for(;;)
    {
        r.run();
    }
#endif

    stop_threads = true;

    t1.join();
    t2.join();
}
