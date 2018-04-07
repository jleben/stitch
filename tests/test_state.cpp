#include "../stitch/state.h"
#include "../testing/testing.h"

#include <thread>
#include <sstream>

using namespace Stitch;
using namespace Testing;
using namespace std;

bool test_observer_before_connecting()
{
    Test test;

    {
        State_Observer<int> observer;

        for (int i = 0; i < 5; ++i)
        {
            test.assert("Value is 0.", observer.value() == 0);
            observer.load();
        }
    }

    {
        State_Observer<int> observer(5);

        for (int i = 0; i < 5; ++i)
        {
            test.assert("Value is 5.", observer.value() == 5);
            observer.load();
        }
    }

    return test.success();
}

bool test_state_before_connecting()
{
    Test test;

    {
        State<int> state;

        for (int i = 0; i < 5; ++i)
        {
            state.value() = i;
            test.assert("Value was written.", state.value() == i);
            state.store();
        }
    }

    {
        State<int> state(9);

        for (int i = 0; i < 5; ++i)
        {
            state.value() = i;
            test.assert("Value was written.", state.value() == i);
            state.store();
        }
    }

    return test.success();
}

bool test_value_after_connecting()
{
    Test test;

    {
        State<int> state;
        State_Observer<int> observer(222);

        test.assert("1: Read before connecting.", observer.value() == 222);
        test.assert("1: Load before connecting.", observer.load() == 222);

        observer.connect(state);

        int v;
        test.assert("1: Read after connecting: " + to_string(observer.value()), observer.value() == 222);
        v = observer.load();
        test.assert("1: Load after connecting: " + to_string(v), v == 0);
        test.assert("1: Read after connecting and loading.", observer.value() == 0);
    }
    {
        State<int> state(111);
        State_Observer<int> observer;

        test.assert("2: Read before connecting.", observer.value() == 0);
        test.assert("2: Load before connecting.", observer.load() == 0);

        observer.connect(state);

        int v;
        test.assert("2: Read after connecting: " + to_string(observer.value()), observer.value() == 0);
        v = observer.load();
        test.assert("2: Load after connecting: " + to_string(v), v == 111);
        test.assert("2: Read after connecting and loading.", observer.value() == 111);
    }

    return test.success();
}

bool test_store_load()
{
    Test test;

    State<int> state;
    State_Observer<int> observer;

    observer.connect(state);

    for (int i = 0; i < 10; ++i)
    {
        state.value() = i;
        state.store();

        test.assert("write(value) + store() + load()", observer.load() == i);
        test.assert("read()", observer.value() == i);

        state.store(i+100);

        test.assert("store(value) + load()", observer.load() == i+100);
        test.assert("read()", observer.value() == i+100);
    }

    return test.success();
}

bool test_double_store_load()
{
    Test test;

    State<int> state;
    State_Observer<int> observer;

    observer.connect(state);

    for (int i = 0; i < 10; ++i)
    {
        state.value() = i;
        state.store();
        state.value() = i + 100;
        state.store();

        test.assert("Loaded value.", observer.load() == i + 100);
        test.assert("Read value.", observer.value() == i + 100);
        test.assert("Loaded value.", observer.load() == i + 100);
        test.assert("Read value.", observer.value() == i + 100);
    }

    return test.success();
}

bool test_notification()
{
    Test test;

    State<int> state1;
    State<int> state2;
    State_Observer<int> observer1;
    State_Observer<int> observer2;

    observer1.connect(state2);
    observer2.connect(state1);

    thread t1([&]()
    {
        state1.value() = 100;
        state1.store();

        for (int i = 0; i < 100; ++i)
        {
            wait(observer1.changed());
            int v = observer1.load();

            ostringstream msg;
            msg << "1: Iteration " << i << " received " << v;
            test.assert(msg.str(), v == i);

            state1.value() = 100 + i + 1;
            state1.store();
        }
    });

    thread t2([&]()
    {
        for (int i = 0; i < 100; ++i)
        {
            wait(observer2.changed());
            int v = observer2.load();

            ostringstream msg;
            msg << "2: Iteration " << i << " received " << v;
            test.assert(msg.str(), v == 100 + i);

            state2.value() = i;
            state2.store();
        }
    });

    t1.join();
    t2.join();

    return test.success();
}

Test_Set state_tests()
{
    return {
        { "state-before-connect", test_state_before_connecting },
        { "observer-before-connect", test_observer_before_connecting },
        { "value-after-connect", test_value_after_connecting },
        { "store-load", test_store_load },
        { "double-store-load", test_double_store_load },
        { "notification", test_notification },
    };
}

