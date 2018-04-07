#include "../stitch/hazard_pointers.h"
#include "../testing/testing.h"

#include <string>
#include <thread>
#include <vector>
#include <chrono>

using namespace Stitch;
using namespace Testing;

using namespace std;

using Detail::Hazard_Pointers;
using Detail::Hazard_Pointer;

bool test_stress_allocation()
{
    Test test;

    using clock = chrono::steady_clock;

    auto start = clock::now();

    chrono::seconds duration(1);

    auto thread_func = [&](int)
    {
        int one = 1;
        int two = 2;
        int three = 3;
        int four = 4;
        bool ok = true;
        bool count_ok = true;

        while(clock::now() - start < duration)
        {
            int sum = 0;
            int rep_count = 1000;

            for (int i = 0; i < rep_count; ++i)
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

                sum += *p1.pointer.load();
                sum += *p2.pointer.load();
                sum += *p3.pointer.load();
                sum += *p4.pointer.load();

                p1.release();
                p2.release();
                p3.release();
                p4.release();
            }

            count_ok &= sum == 10 * rep_count;
        }

        test.assert("Allocation consistent.", ok);
        test.assert("Sum consistent.", count_ok);
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

    vector<Hazard_Pointer<int>*> hps;

    try
    {
        for (i = 0; i < Detail::Hazard_Pointers::H + 1; ++i)
        {
            auto & hp = Detail::Hazard_Pointers::acquire<int>();
            hps.push_back(&hp);
        }
    }
    catch (std::runtime_error&)
    {
        exception_thrown = true;
    }

    test.assert("Exception was thrown.", exception_thrown);
    test.assert("Exception was thrown at index " + to_string(i),
                i == Detail::Hazard_Pointers::H);

    for (auto * hp : hps)
    {
        hp->release();
    }

    return test.success();
}

bool test_reclamation()
{
    using Detail::Hazard_Pointers;
    using Detail::Hazard_Pointer;

    Test test;

    static atomic<int> created_count { 0 };
    static atomic<int> deleted_count { 0 };

    struct Element
    {
        Element() { created_count.fetch_add(1); }
        Element(const Element &): Element() {}
        ~Element() { deleted_count.fetch_add(1); }
    };

    int repetitions = 100;

    for (int rep = 0; rep < repetitions; ++rep)
    {
        vector<Hazard_Pointer<Element>*> hps;

        created_count = 0;
        deleted_count = 0;

        for (int i = 0; i < 5; ++i)
        {
            Element * p = new Element;
            auto & hp = Hazard_Pointers::acquire<Element>();
            hp.pointer = p;
            hps.emplace_back(&hp);

            Hazard_Pointers::reclaim(p);
        }

        for (int i = 0; i < Hazard_Pointers::H && deleted_count == 0; ++i)
        {
            Element * p = new Element;
            Hazard_Pointers::reclaim(p);
        }

        test.assert_critical("Something was deleted.", deleted_count > 0);
        test.assert_critical("Hazardous pointers were not deleted.", deleted_count == created_count - 5);

        int last_deleted_count = deleted_count;

        for (auto * hp : hps)
        {
            hp->pointer = nullptr;
            hp->release();
        }

        for (int i = 0; i < Hazard_Pointers::H && deleted_count == last_deleted_count; ++i)
        {
            Element * p = new Element;
            Hazard_Pointers::reclaim(p);
        }

        test.assert_critical("Everything was deleted.", deleted_count == created_count);
    }

    return test.success();
}

Test_Set hazard_pointers_tests()
{
    return {
        { "stress-allocation", test_stress_allocation },
        { "reclamation", test_reclamation },
        { "over-allocation", test_over_allocation },
    };
}
