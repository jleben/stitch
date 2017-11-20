#include "../concurrency/mpsc_queue.h"
#include "../testing/testing.h"

using namespace Reactive;
using namespace std;

static bool test()
{
    Testing::Test test;

    test.assert("Lockfree.", MPSC_Queue<int>::is_lockfree());

    MPSC_Queue<int> q (10);

    for (int rep = 0; rep < 3; ++rep)
    {
        for (int i = 0; i < 7; ++i)
        {
            test.assert("Not full.", !q.full());

            bool ok = q.push(i);
            test.assert("Pushed.", ok);
        }

        for (int i = 0; i < 7; ++i)
        {
            test.assert("Not empty.", !q.empty());

            int v;
            bool ok = q.pop(v);
            test.assert("Popped.", ok);
            if (ok)
                test.assert("Popped " + to_string(v), v == i);
        }
    }

    return test.success();
}

static bool stress_test()
{
    Testing::Test test;

    MPSC_Queue<int> q(50);

    atomic<bool> quit { false };

    auto producer = ([&](bool first)
    {
        int v = 1;

        while(!quit)
        {
            while(!quit)
            {
                //printf("%s %d\n", first ? "1: " : "2: ", v);
                bool ok = q.push(first ? v : v + 1000);
                if (ok)
                    v = (v + 1) & 0xFF;
                else
                    break;
            }

            this_thread::yield();
        }
    });

    thread p1(producer, true);
    thread p2(producer, false);

    bool work = true;
    int v1 = 1;
    int v2 = 1;
    int v;

    auto start = chrono::steady_clock::now();

    while(work)
    {
        this_thread::yield();

        work &= chrono::steady_clock::now() - start < chrono::seconds(5);

        while(work && q.pop(v))
        {
            work &= chrono::steady_clock::now() - start < chrono::seconds(5);

            if (v < 1000)
            {
                bool correct = v1 == v;
                test.assert("Stream 1 = " + to_string(v)
                            + ", expected " + to_string(v1),
                            correct);
                v1 = (v + 1) & 0xFF;
                //printf("1: Next: %d\n", v1);
                work &= correct;
            }
            else
            {
                v -= 1000;
                bool correct = v2 == v;
                test.assert("Stream 2 = " + to_string(v)
                            + ", expected " + to_string(v2),
                            correct);
                v2 = (v + 1) & 0xFF;
                //printf("2: Next: %d\n", v2);
                work &= correct;
            }
        }
    }

    quit = true;
    p1.join();
    p2.join();

    return test.success();
}

Testing::Test_Set mpsc_queue_tests()
{
    return {
        { "test", test },
        { "stress", stress_test },
    };
}
