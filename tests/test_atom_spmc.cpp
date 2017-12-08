#include "../stitch/atom_spmc.h"
#include "../testing/testing.h"

#include <thread>
#include <chrono>

using namespace Stitch;
using namespace Testing;
using namespace std;

struct Data
{
    int a = 1;
    int b = 2;
    int c = 3;
};

static bool test_basic()
{
    Test test;

    {
        SPMC_Atom<Data> atom;
        auto data = atom.load();
        test.assert("Correct default-constructed value.",
                    data.a == 1 && data.b == 2 && data.c == 3);
    }

    {
        SPMC_Atom<Data> atom({ 3, 2, 1 });
        auto data = atom.load();
        test.assert("Correct initial value.",
                    data.a == 3 && data.b == 2 && data.c == 1);
    }

    {
        SPMC_Atom<Data> atom;
        atom.store({ 4, 5, 6 });
        auto data = atom.load();
        test.assert("Correct stored and loaded value.",
                    data.a == 4 && data.b == 5 && data.c == 6);
    }

    return test.success();
}

static bool test_stress()
{
    Test test;

    SPMC_Atom<Data> atom({ 0, 0, 0});

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
        { "basic", test_basic },
        { "stress", test_stress }
    };
}

