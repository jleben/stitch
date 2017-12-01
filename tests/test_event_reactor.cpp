#include "../stitch/events.h"
#include "../stitch/signal.h"
#include "../testing/testing.h"

using namespace Stitch;
using namespace Testing;
using namespace std;

static bool test_quit()
{
    Test test;

    Event_Reactor reactor;

    Signal signal1;
    Signal signal2;

    int x1 = 0;
    int x2 = 0;

    reactor.subscribe(signal1.event(), [&](){ ++x1; reactor.quit(); });
    reactor.subscribe(signal2.event(), [&](){ ++x2; });

    signal1.notify();
    signal2.notify();

    reactor.run();

    test.assert("Only one signal is handled.", x1 == 1 && x2 == 0);

    return test.success();
}

Test_Set event_reactor_tests()
{
    return {
        { "quit", test_quit },
    };
}
