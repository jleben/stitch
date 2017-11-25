#include "../concurrency/lockfree_set.h"
#include "../testing/testing.h"

#include <unordered_set>
#include <chrono>
#include <thread>
#include <sstream>

using namespace Testing;
using namespace Reactive;
using namespace std;

static bool empty()
{
    Test test;

    Set<int> set;

    test.assert("Set empty.", set.empty());

    set.insert(1);

    test.assert("Set not empty.", !set.empty());

    return test.success();
}

static bool contains()
{
    Test test;

    Set<int> set;

    for (int i = 0; i < 10; ++i)
    {
        set.insert(i);
    }

    for (int i = 0; i < 10; ++i)
    {
        test.assert("Set contains " + to_string(i), set.contains(i));
    }

    test.assert("Set does not contain -1.", !set.contains(-1));

    for (int i : { 0, 4, 5, 3, 7 })
    {
        bool ok = set.remove(i);
        test.assert("Element " + to_string(i) + " removed.", ok);
    }

    for (int i : { 1, 2, 6, 8, 9 })
    {
        test.assert("Set contains " + to_string(i), set.contains(i));
    }

    for (int i : { 0, 3, 4, 5, 7 })
    {
        test.assert("Set does not contain " + to_string(i), !set.contains(i));
    }

    return test.success();
}

static bool iteration()
{
    Test test;

    Set<int> set;

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

    for (int i : { 0, 4, 5, 3, 7 })
    {
        bool ok = set.remove(i);
        test.assert("Element " + to_string(i) + " removed.", ok);
    }

    {
        vector<int> elements;

        set.for_each([&](int i){
            elements.push_back(i);
        });

        test.assert("Set size is 5.", elements.size() == 5);

        unordered_set<int> unique_elements;

        for (auto & e : elements)
        {
            bool is_unique;
            tie(ignore, is_unique) = unique_elements.emplace(e);
            test.assert("Element is unique.", is_unique);
        }

        for (int i : { 1, 2, 6, 8, 9 })
        {
            test.assert("Set iterates over " + to_string(i), unique_elements.find(i) != unique_elements.end());
        }
    }

    return test.success();
}

static bool reclamation()
{
    Test test;

    static atomic<int> elem_count { 0 };

    struct Element
    {
        int x;

        Element(): Element(0) {}
        Element(int x): x(x) { elem_count.fetch_add(1); }
        Element(const Element & other): Element(other.x) {}
        ~Element() { elem_count.fetch_sub(1); }
        Element & operator=(const Element & other){ x = other.x; return *this; }
        bool operator==(const Element & other) { return x == other.x; }
    };

    Set<Element> set;

    for (int i = 0; i < 2 * Hazard_Pointers::H; ++i)
    {
        set.insert(i);
        test.assert("Set contains " + to_string(i), set.contains(i));
    }

    // NOTE: There's always one additional element: the set's head.
    test.assert("Element count " + to_string(elem_count)
                + " = " + to_string(2*Hazard_Pointers::H + 1),
                elem_count == 2 * Hazard_Pointers::H + 1);

    thread t1([&]()
    {
        for (int i = 0; i < Hazard_Pointers::H; ++i)
        {
            int val = 2*i;
            bool removed = set.remove(val);
            test.assert("Element " + to_string(val) + " removed.", removed);
        }
    });

    thread t2([&]()
    {
        for (int i = 0; i < Hazard_Pointers::H; ++i)
        {
            int val = 2*i + 1;
            bool removed = set.remove(val);
            test.assert("Element " + to_string(val) + " removed.", removed);
        }
    });

    t1.join();
    t2.join();

    // NOTE: There's always one element: the set's head.
    test.assert("Element count " + to_string(elem_count) + " = 1.", elem_count == 1);

    return test.success();
}

static bool stress()
{
    Test test;

    Set<int> set;

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

                // FIXME: This test is invalid.
                // It is possible for set.for_each
                // to iterate over the same value multiple times,
                // if the value was remove and reinserted in the meantime.

                bool is_unique;
                tie(ignore, is_unique) = unique_elements.emplace(e);

                test.assert_critical("Element is unique: " + to_string(e), is_unique);
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
        { "empty", empty },
        { "contains", contains },
        { "iteration", iteration },
        { "reclamation", reclamation },
        { "stress", stress },
    };
}
