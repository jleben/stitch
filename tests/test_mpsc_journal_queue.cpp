#include "../common/mpsc_journal_queue.hpp"
#include "../testing/testing.h"

using namespace Reactive;
using namespace std;

bool test()
{
    Testing::Test test;

    test.assert("Lockfree.", MPSC_Journal_Queue<int>::is_lockfree());

    MPSC_Journal_Queue<int> q (10);

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

int main(int argc, char * argv[])
{
    Testing::Test_Set tests = {
        { "test", test }
    };

    return Testing::run(tests, argc, argv);
}

