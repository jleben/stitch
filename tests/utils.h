#pragma once

#include <string>
#include <chrono>
#include <iostream>
#include <functional>
#include <vector>
#include <utility>
#include <initializer_list>
#include <atomic>

namespace Testing {

using std::string;
using std::function;
using std::vector;
using std::pair;
using std::atomic;

inline double time_since(const std::chrono::steady_clock::time_point & start)
{
    using namespace std;
    using namespace std::chrono;

    auto d = chrono::steady_clock::now() - start;
    double s = duration_cast<duration<double>>(d).count();
    return s;
}

inline
bool assert(const string & message, bool value)
{
    printf("%s: %s\n", (value ? "OK" : "ERROR"), message.c_str());
}

class Test
{
public:
    void assert(const string & message, bool value)
    {
        Testing::assert(message, value);

        if (!value)
            d_success = false;
    }

    void assert_critical(const string & message, bool value)
    {
        Testing::assert(message, value);

        if (!value)
            d_success = false;

        throw std::runtime_error("Critical assertion failed: " + message);
    }

    bool success() const { return d_success; }

private:
    atomic<bool> d_success { true };
};

class Set
{
public:
    using Func = function<bool()>;

    Set(std::initializer_list<pair<string,Func>> tests):
        d_tests(tests)
    {}

    bool run()
    {
        bool all_ok = true;

        using namespace std;
        for (auto & test : d_tests)
        {
            cerr << endl << "-- " << test.first << endl;
            bool ok = test.second();
            cerr << "-- " << (ok ? "PASSED" : "FAILED") << endl;
            all_ok &= ok;
        }

        return (all_ok ? 0 : 1);
    }

private:
    vector<pair<string,Func>> d_tests;
};

}
