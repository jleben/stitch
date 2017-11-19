#include "../data_structs/lockfree_set.h"
#include "../testing/testing.h"

#include <unordered_set>
#include <chrono>
#include <thread>
#include <sstream>

using namespace Testing;
using namespace Concurrent;
using namespace std;

static bool contains()
{
    Test test;

    Lockfree::Set<int> set;

    for (int i = 0; i < 10; ++i)
    {
        set.insert(i);
    }

    for (int i = 0; i < 10; ++i)
    {
        test.assert("Set contains " + to_string(i), set.contains(i));
    }

    test.assert("Set does not contain -1.", !set.contains(-1));

    set.remove(4);
    set.remove(5);
    set.remove(3);
    set.remove(7);

    for (int i : { 0, 1, 2, 6, 8, 9 })
    {
        test.assert("Set contains " + to_string(i), set.contains(i));
    }

    for (int i : { 3, 4, 5, 7 })
    {
        test.assert("Set does not contain " + to_string(i), !set.contains(i));
    }

    return test.success();
}

static bool iteration()
{
    Test test;

    Lockfree::Set<int> set;

    for (int i = 0; i < 10; ++i)
    {
        set.insert(i);
    }

    // Repeat, to make sure set does not insert duplicates
    for (int i = 0; i < 10; ++i)
    {
        set.insert(i);
    }

    {
        vector<int> elements;

        set.for_each([&](int i){
            elements.push_back(i);
        });

        test.assert("Set size is 10.", elements.size() == 10);

        unordered_set<int> unique_elements;

        for (auto & e : elements)
        {
            bool is_unique;
            tie(ignore, is_unique) = unique_elements.emplace(e);
            test.assert("Element is unique.", is_unique);
        }

        for (int i = 0; i < 10; ++i)
        {
            test.assert("Set iterates over " + to_string(i), unique_elements.find(i) != unique_elements.end());
        }
    }

    set.remove(4);
    set.remove(5);
    set.remove(3);
    set.remove(7);

    {
        vector<int> elements;

        set.for_each([&](int i){
            elements.push_back(i);
        });

        test.assert("Set size is 6.", elements.size() == 6);

        unordered_set<int> unique_elements;

        for (auto & e : elements)
        {
            bool is_unique;
            tie(ignore, is_unique) = unique_elements.emplace(e);
            test.assert("Element is unique.", is_unique);
        }

        for (int i : { 0, 1, 2, 6, 8, 9 })
        {
            test.assert("Set iterates over " + to_string(i), unique_elements.find(i) != unique_elements.end());
        }
    }

    return test.success();
}

static bool stress()
{
    Test test;

    Lockfree::Set<int> set;

    atomic<bool> done;

    thread modifier([&]()
    {
        while(!done)
        {
            for (int i = 0; i < 100; ++i)
            {
                set.insert(i);
            }
            for (int i = 50; i < 75; ++i)
            {
                set.remove(i);
            }
            for (int i = 99; i >= 75; --i)
            {
                set.remove(i);
            }
            for (int i = 49; i >= 25; --i)
            {
                set.remove(i);
            }
            for (int i = 0; i < 25; ++i)
            {
                set.remove(i);
            }
        }
    });

    auto start = chrono::steady_clock::now();

    try
    {
        while(chrono::steady_clock::now() - start < chrono::seconds(1))
        {
            vector<int> elements;

            set.for_each([&](int i){
                elements.push_back(i);
            });

            unordered_set<int> unique_elements;

            for (auto & e : elements)
            {
                {
                    ostringstream msg;
                    msg << "Element " << e << " is in correct range.";
                    test.assert_critical(msg.str(), e >= 0 && e < 100);
                }

                bool is_unique;
                tie(ignore, is_unique) = unique_elements.emplace(e);

                test.assert_critical("Element is unique.", is_unique);
            }
        }
    }
    catch(std::runtime_error & e)
    {
        cerr << e.what() << endl;
    }

    done = true;
    modifier.join();

    return test.success();
}

Test_Set lockfree_set_tests()
{
    return
    {
        { "contains", contains },
        { "iteration", iteration },
        { "stress", stress },
    };
}
