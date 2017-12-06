#include "../stitch/queue_mpmc_waitfree.h"
#include "../testing/testing.h"

using namespace Stitch;
using namespace std;

static bool test()
{
    Testing::Test test;

    test.assert("Lockfree.", Waitfree_MPMC_Queue<int>::is_lockfree());

    Waitfree_MPMC_Queue<int> q (10);

    for (int rep = 0; rep < 3; ++rep)
    {
        for (int i = 0; i < 7; ++i)
        {
            q.push(i);
        }

        for (int i = 0; i < 7; ++i)
        {
            while(q.empty())
                q.event().wait();

            int v;
            q.pop(v);
            test.assert("Popped " + to_string(v), v == i);
        }
    }

    return test.success();
}

Testing::Test_Set waitfree_mpmc_queue_tests()
{
    return {
        { "test", test }
    };
}
