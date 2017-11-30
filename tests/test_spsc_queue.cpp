#include "../stitch/spsc_queue.h"
#include "../testing/testing.h"

#include <thread>
#include <chrono>

using namespace Stitch;
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

static bool test_bulk()
{
    Testing::Test test;

    SPSC_Queue<int> q(10);

    for (int rep = 0; rep < 6; ++rep)
    {
        int count = q.capacity() - 2;

        {
            vector<int> data(count);
            for(int i = 0; i < count; ++i)
                data[i] = i;

            bool pushed = q.push(count, data.begin());
            test.assert("Pushed.", pushed);
        }

        test.assert("Queue is not empty.", !q.empty());

        {
            vector<int> data(count);

            bool popped = q.pop(count, data.begin());
            test.assert("Popped.", popped);

            for (int i = 0; i < count; ++i)
                test.assert("Got " + to_string(data[i]), data[i] == i);
        }

        test.assert("Queue is empty.", q.empty());
    }

    {
        vector<int> data(q.capacity() + 1);
        test.assert("Can't pop when empty.", !q.pop(1, data.begin()));
        test.assert("Can't push more than capcity.", !q.push(q.capacity() + 1, data.begin()));
    }

    return test.success();
}

static bool test_stress()
{
    Testing::Test test;

    int rep_count = 100;
    int rep_size = 10;

    SPSC_Queue<int> q(rep_size * 5);

    thread producer = thread([&]()
    {
        for (int rep = 0; rep < rep_count; ++rep)
        {
            test.assert("Queue is empty", q.empty());

            for (int i = 0; i < rep_size; ++i)
            {
                test.assert("Pushed " + to_string(i), q.push(i));
            }

            this_thread::sleep_for(chrono::milliseconds(10));
        }
    });

    for (int rep = 0; rep < rep_count; ++rep)
    {
        for (int i = 0; i < rep_size; ++i)
        {
            while (q.empty())
                this_thread::sleep_for(chrono::milliseconds(5));

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
        { "full_empty", test_full_empty },
        { "single_thread", test_single_thread },
        { "bulk", test_bulk },
        { "stress", test_stress }
    };
}
