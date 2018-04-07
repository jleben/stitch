#include "../stitch/hazard_pointers.h"
#include "../testing/testing.h"

#include <string>
#include <thread>

using namespace Stitch;
using namespace Testing;

using namespace std;

bool test_allocation()
{
    Test test;

    int repetitions = 10000;

    auto thread_func = [&](int)
    {
        int one = 1;
        int two = 2;
        int three = 3;
        int four = 4;
        bool ok = true;

        int count = 0;

        for (int i = 0; i < repetitions; ++i)
        {
            auto & p1 = Detail::Hazard_Pointers::acquire<int>();
            auto & p2 = Detail::Hazard_Pointers::acquire<int>();
            auto & p3 = Detail::Hazard_Pointers::acquire<int>();
            auto & p4 = Detail::Hazard_Pointers::acquire<int>();

            // Assign something
            p1.pointer = &one;
            p2.pointer = &two;
            p3.pointer = &three;
            p4.pointer = &four;

            ok &= *p1.pointer.load() == 1;
            ok &= *p2.pointer.load() == 2;
            ok &= *p3.pointer.load() == 3;
            ok &= *p4.pointer.load() == 4;

            count += *p1.pointer.load();
            count += *p2.pointer.load();
            count += *p3.pointer.load();
            count += *p4.pointer.load();

            p1.release();
            p2.release();
            p3.release();
            p4.release();
        }

        test.assert("Allocation consistent.", ok);
        test.assert("Total count ok.", count == repetitions * 10);
    };

    thread t1(thread_func, 1);
    thread t2(thread_func, 2);

    t1.join();
    t2.join();

    return test.success();
}

bool test_over_allocation()
{
    Test test;

    bool exception_thrown = false;
    int i;

    try
    {
        for (i = 0; i < Detail::Hazard_Pointers::H + 1; ++i)
        {
            Detail::Hazard_Pointers::acquire<int>();
        }
    }
    catch (std::runtime_error&)
    {
        exception_thrown = true;
    }

    test.assert("Exception was thrown.", exception_thrown);
    test.assert("Exception was thrown at index " + to_string(i),
                i == Detail::Hazard_Pointers::H);

    return test.success();
}

Test_Set hazard_pointers_tests()
{
    return {
        { "allocation", test_allocation },
        { "over-allocation", test_over_allocation },
    };
}
