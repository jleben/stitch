#pragma once

#include <string>
#include <chrono>

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

}
