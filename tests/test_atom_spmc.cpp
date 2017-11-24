#include "../concurrency/atom_spmc.h"
#include "../testing/testing.h"

#include <thread>
#include <chrono>

using namespace Reactive;
using namespace Testing;
using namespace std;

struct Data
{
    int a = 0;
    int b = 0;
    int c = 0;
};

static bool test_stress()
{
    Test test;

    SPMC_Atom<Data> atom;

    atomic<bool> work { true };

    thread producer([&]()
    {
        int i = 0;
        Data d;

        while(work)
        {
            ++i;
            d.a = d.b = d.c = i;
            atom.store(d);
        }
    });

    auto consumer = [&]()
    {
        Data d;

        while(work)
        {
            d = atom.load();
            int v = d.a;
            bool ok = d.a == v && d.b == v && d.c == v;
            test.assert("a = b = c", ok);
        }
    };

    thread c1(consumer);
    thread c2(consumer);

    this_thread::sleep_for(chrono::milliseconds(1000));

    work = false;

    producer.join();
    c1.join();
    c2.join();

    return test.success();
}

Testing::Test_Set spmc_atom_tests()
{
    return {
        { "stress", test_stress }
    };
}

