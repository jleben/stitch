#pragma once

#include <string>
#include <chrono>
#include <iostream>

namespace Test {

using std::string;

inline string time_since(const std::chrono::steady_clock::time_point & start)
{
    using namespace std;
    using namespace std::chrono;

    auto d = chrono::steady_clock::now() - start;
    double s = duration_cast<duration<double>>(d).count();
    return to_string(s);
}

inline
void assert(const string & message, bool value)
{
    using namespace std;

    cerr << (value ? "OK: " : "ERROR: ") << message << endl;
}

}
