#include "../testing/testing.h"
#include "../experimental/variable.h"

#include <thread>
#include <chrono>

using namespace Stitch;
using namespace Testing;
using namespace std;

static bool test_basic()
{
    Test test;

    Variable<int> var1;
    Variable<int> var2;

    VariableReader<int> reader(999);

    var1.set(1);
    var2.set(2);

    test.assert("Reader gets 999.", reader.get() == 999);

    reader.connect(var1);

    test.assert("Reader gets 1.", reader.get() == 1);

    reader.connect(var2);

    test.assert("Reader gets 2.", reader.get() == 2);

    var1.set(11);

    test.assert("Reader gets 2.", reader.get() == 2);

    var2.set(22);

    test.assert("Reader gets 22.", reader.get() == 22);

    reader.disconnect();

    test.assert("Reader gets 999.", reader.get() == 999);

    // Test auto-disconnect writer.
    {
        Variable<int> var3;

        var3.set(3);

        reader.connect(var3);

        test.assert("Reader gets 3.", reader.get() == 3);
    }

    test.assert("Reader gets 999.", reader.get() == 999);

    // Test auto-disconnect reader.

    {
        VariableReader<int> reader2;

        reader2.connect(var1);
    }

    // Just make sure there's no crash.

    var1.set(111);

    return test.success();
}

static bool test_event()
{
    Test test;

    Variable<int> writer;
    VariableReader<int> reader(0);
    Signal signal;

    writer.set(1);

    reader.connect(writer);

    thread read_thread([&]()
    {
        wait(reader.changed());

        test.assert("Reader gets 2.", reader.get() == 2);

        reader.disconnect();

        auto start = chrono::steady_clock::now();

        wait({ reader.changed(), signal.event() });

        auto end = chrono::steady_clock::now();
        auto duration = end - start;

        test.assert("Disconnected reader was not notified.",
                    duration > chrono::milliseconds(80) &&
                    duration < chrono::milliseconds(120));
    });

    this_thread::sleep_for(chrono::milliseconds(100));

    writer.set(2);

    this_thread::sleep_for(chrono::milliseconds(50));

    writer.set(3);

    this_thread::sleep_for(chrono::milliseconds(50));

    signal.notify();

    read_thread.join();

    return test.success();
}

Testing::Test_Set variable_tests()
{
    return {
        { "basic", test_basic },
        { "event", test_event },
    };
}


