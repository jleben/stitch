#include "../stitch/multiset.h"
#include "../testing/testing.h"

#include <thread>
#include <sstream>
#include <iostream>
#include <cstdint>

using namespace Stitch;
using namespace Testing;
using namespace std;

bool test()
{
    Test test;

    Multiset<int> mset;
    cout << "Insert" << endl;
    mset.insert(1);
    cout << "Insert" << endl;
    mset.insert(2);
    cout << "Insert" << endl;
    mset.insert(3);

    test.assert("Can't remove 123.", !mset.remove(123));

    for(auto & i : mset)
    {
        cout << i << endl;
    }

    test.assert("Remove 2.", mset.remove(2));

    for(auto & i : mset)
    {
        cout << i << endl;
    }

    test.assert("Remove 3.", mset.remove(3));

    for(auto & i : mset)
    {
        cout << i << endl;
    }

    test.assert("Remove 1.", mset.remove(1));

    for(auto & i : mset)
    {
        cout << i << endl;
    }

    test.assert("Can't remove 456.", !mset.remove(456));

    test.assert("Insert 5.", mset.insert(5));
    test.assert("Insert 5.", mset.insert(5));
    test.assert("Insert 5.", mset.insert(5));

    for(auto & i : mset)
    {
        cout << i << endl;
    }

    return test.success();
}

Test_Set multiset_tests()
{
    return {
        { "test", test },
    };
}
