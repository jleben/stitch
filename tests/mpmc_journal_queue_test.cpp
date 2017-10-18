#include "../common/mpmc_journal_queue.hpp"
#include "../testing/testing.h"

using namespace Reactive;
using namespace std;

bool test()
{
    Testing::Test test;

    MPMC_Journal_Queue<int> q (10);

    for (int i = 0; i < 5; ++i)
    {
        q.push(i);
    }

    for (int i = 0; i < 5; ++i)
    {
        while(q.empty())
            q.event().wait();

        int v = q.pop();
        test.assert("Popped " + to_string(v), v == i);
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
