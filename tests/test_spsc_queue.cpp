#include "../concurrency/spsc_queue.h"
#include "../testing/testing.h"

#include <thread>
#include <chrono>

using namespace Reactive;
using namespace std;

static bool test_is_lockfree()
{
    Testing::Test test;
    test.assert("Lockfree.", SPSC_Queue<int>::is_lockfree());
    return test.success();
}

static bool test_full_empty()
{
    Testing::Test test;

    SPSC_Queue<int> q(10);

    test.assert("Queue empty.", q.empty());
    test.assert("Queue not full.", !q.full());

    {
        int v;
        test.assert("Can not pop empty queue.", !q.pop(v));
    }

    for (int i = 0; i < q.capacity(); ++i)
    {
        test.assert("Pushed " + to_string(i) + " successfully.", q.push(i));
    }

    test.assert("Queue full.", q.full());
    test.assert("Queue not empty.", !q.empty());

    test.assert("Can not push to empty queue.", !q.push(111));

    for (int i = 0; i < q.capacity(); ++i)
    {
        int v;
        bool ok = q.pop(v);
        test.assert("Popped successfully.", ok);
        if (ok)
            test.assert("Popped " + to_string(v), v == i);
    }

    return test.success();
}

static bool test_single_thread()
{
    Testing::Test test;

    SPSC_Queue<int> q(10);

    for (int rep = 0; rep < 2; ++rep)
    {
        test.assert("Queue empty.", q.empty());

        for (int i = 0; i < 7; ++i)
        {
            test.assert("Pushed " + to_string(i), q.push(i));
        }

        for (int i = 0; i < 7; ++i)
        {
            int v;
            bool ok = q.pop(v);
            test.assert("Popped.", ok);
            if (ok)
                test.assert("Popped value = " + to_string(v), v == i);
        }
    }

    test.assert("Queue empty.", q.empty());
    test.assert("Queue not full.", !q.full());

    return test.success();
}

static bool test_multi_thread()
{
    Testing::Test test;

    SPSC_Queue<int> q(10);

    thread producer = thread([&]()
    {
        for (int rep = 0; rep < 3; ++rep)
        {
            test.assert("Queue is empty", q.empty());

            for (int i = 0; i < 7; ++i)
            {
                test.assert("Pushed " + to_string(i), q.push(i));
            }

            this_thread::sleep_for(chrono::milliseconds(100));
        }
    });

    for (int rep = 0; rep < 3; ++rep)
    {
        for (int i = 0; i < 7; ++i)
        {
            while (q.empty())
                wait(q.write_event());

            int v;
            bool ok = q.pop(v);
            test.assert("Popped.", ok);
            if (ok)
                test.assert("Popped value = " + to_string(v), v == i);
        }
    }

    producer.join();

    test.assert("Queue empty.", q.empty());
    test.assert("Queue not full.", !q.full());

    return test.success();
}

Testing::Test_Set spsc_queue_tests()
{
    return {
        { "lockfree", test_is_lockfree },
        { "test_full_empty", test_full_empty },
        { "test_single_thread", test_single_thread },
        { "test_multi_thread", test_multi_thread }
    };
}
