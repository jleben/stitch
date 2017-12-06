#include "../stitch/queue_mpmc_lockfree.h"
#include "../testing/testing.h"

#include <thread>
#include <chrono>

using namespace Testing;
using namespace Stitch;
using namespace std;

static bool test_one()
{
    Test test;

    Lockfree_MPMC_Queue<int> q(10000);

    atomic<bool> stop { false };

    thread p([&]()
    {
        int i = 0;
        while(!stop)
        {
            if (!q.push(i))
            {
                this_thread::yield();
                continue;
            }
            ++i;
        }
    });

    bool error = false;

    thread c([&]()
    {
        int i = 0;
        while(!stop)
        {
            int v;
            if (!q.pop(v))
            {
                this_thread::yield();
                continue;
            }
            if (v != i)
            {
                error = true;
            }
            ++i;
        }
    });

    this_thread::sleep_for(chrono::seconds(3));

    stop = true;

    p.join();
    c.join();

    test.assert("No error.", !error);

    return test.success();
}

static bool test_many()
{
    Test test;

    Lockfree_MPMC_Queue<int> q(10000);

    atomic<bool> stop { false };

    auto producer = [&](int step)
    {
        int v = 0;
        while(!stop)
        {
            if(!q.push(v))
            {
                this_thread::yield();
                continue;
            }

            v += step;
        }
    };

    bool error = false;

    auto consumer = [&]()
    {
        while(!stop)
        {
            int v;
            if(!q.pop(v))
            {
                this_thread::yield();
                continue;
            }

            bool ok = v % 5 == 0 || v % 7 == 0;
            if (!ok)
                error = true;
        }
    };

    thread p1(producer, 5);
    thread p2(producer, 7);
    thread c1(consumer);
    thread c2(consumer);

    printf("Sleeping...\n");

    this_thread::sleep_for(chrono::seconds(3));

    stop = true;

    printf("Stopping...\n");

    p1.join();
    printf("p1 joined\n");
    p2.join();
    printf("p2 joined\n");
    c1.join();
    printf("c1 joined\n");
    c2.join();
    printf("c2 joined\n");

    test.assert("No error.", !error);

    return test.success();
}

Testing::Test_Set lockfree_mpmc_queue_tests()
{
    return {
        { "one", test_one },
        { "many", test_many },
    };
}

