#include "../common/stream_gen.hpp"
#include "../testing/testing.h"

#include <string>

using namespace Reactive;
using namespace Testing;
using namespace std;

bool test_basic()
{
    Test test;

    Stream_Generator<int> gen;

    vector<int> values;

    gen.subscribe([&](int v){ values.push_back(v); });

    for (int i = 0; i < 10; ++i)
    {
        gen.push(i);
    }

    test.assert("Received " + to_string(values.size()) + " values.", values.size() == 10);

    for (int i = 0; i < 10; ++i)
    {
        test.assert("Value " + to_string(i) + " = " + to_string(i), values[i] == i);
    }

    return test.success();
}

int main(int argc, char * argv[])
{
    Test_Set t = {
        { "basic", test_basic },
    };

    return Testing::run(t, argc, argv);
}

