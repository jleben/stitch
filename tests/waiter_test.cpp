#include "../linux/signal.h"
#include "../linux/waiter.h"
#include "utils.h"

using namespace Reactive;

bool test_next()
{
    Testing::Test test;

    Signal s1;
    Signal s2;

    Waiter w;
    w.add(s1);
    w.add(s2);

    Event * e;

    e = w.next();
    test.assert("No event ready at start.", e == nullptr);

    s1.notify();
    e = w.next();
    test.assert("S1 ready.", e == &s1);

    s2.notify();
    e = w.next();
    test.assert("S2 ready.", e == &s2);

    e = w.next();
    test.assert("Events cleared.", e == nullptr);

    s1.notify();
    s2.notify();
    {
        auto e1 = w.next();
        test.assert("One event ready.", e1 == &s1 || e1 == &s2);
        auto e2 = w.next();
        test.assert("Another event ready.", e2 != e1 && (e2 == &s1 || e2 == &s2));
    }

    e = w.next();
    test.assert("Events cleared.", e == nullptr);

    return test.success();
}

int main(int argc, char *argv[])
{
    Testing::Test_Set t = {
        { "next", test_next }
    };

    return Testing::run(t, argc, argv);
}
