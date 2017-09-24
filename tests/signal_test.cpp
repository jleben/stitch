#include "../linux/signal.h"
#include "../testing/testing.h"

#include <thread>
#include <iostream>

using namespace Reactive;
using namespace std;
using namespace Testing;

bool test_wait()
{
    Signal s;

    thread t1([&](){

        s.notify();

        this_thread::sleep_for(chrono::milliseconds(500));

        s.notify();
    });

    auto start = chrono::steady_clock::now();

    s.wait();

    cout << Testing::time_since(start) << endl;

    s.wait();

    cout << Testing::time_since(start) << endl;

    t1.join();

    return true;
}

int main(int argc, char * argv[])
{
    Test_Set t = {
        { "wait", test_wait }
    };

    return Testing::run(t, argc, argv);
}

