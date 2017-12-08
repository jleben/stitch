#include "../testing/testing.h"
#include "../stitch/notice.h"

#include <thread>
#include <chrono>

using namespace Stitch;
using namespace Testing;
using namespace std;

static bool test_basic()
{
    Test test;

    Notice<int> writer1(1);
    Notice<int> writer2;

    writer2.post(2);

    NoticeReader<int> reader(999);

    test.assert("Reader gets 999.", reader.read() == 999);

    reader.connect(writer1);

    test.assert("Reader gets 1.", reader.read() == 1);

    reader.connect(writer2);

    test.assert("Reader gets 2.", reader.read() == 2);

    writer1.post(11);

    test.assert("Reader gets 2.", reader.read() == 2);

    writer2.post(22);

    test.assert("Reader gets 22.", reader.read() == 22);

    reader.disconnect();

    test.assert("Reader gets 999.", reader.read() == 999);

    // Test auto-disconnecting writer.
    {
        Notice<int> writer3(3);

        reader.connect(writer3);

        test.assert("Reader gets 3.", reader.read() == 3);
    }

    test.assert("Reader gets 999.", reader.read() == 999);

    // Test auto-disconnect reader.

    {
        NoticeReader<int> reader2;

        reader2.connect(writer1);
    }

    // Just make sure there's no crash.

    writer1.post(111);

    return test.success();
}

static bool test_event()
{
    Test test;

    Notice<int> writer;
    NoticeReader<int> reader(0);
    Signal signal;

    writer.post(1);

    reader.connect(writer);

    thread read_thread([&]()
    {
        wait(reader.changed());

        test.assert("Reader gets 2.", reader.read() == 2);

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

    writer.post(2);

    this_thread::sleep_for(chrono::milliseconds(50));

    writer.post(3);

    this_thread::sleep_for(chrono::milliseconds(50));

    signal.notify();

    read_thread.join();

    return test.success();
}

Testing::Test_Set notice_tests()
{
    return {
        { "basic", test_basic },
        { "event", test_event },
    };
}


