#pragma once

#include <array>
#include <atomic>
#include <cstdint>

namespace Reactive {

using std::atomic;
using std::array;

// This class uses versioning to detect contention.
// Harmful contention will be detected as long
// as 'store' is executed less than N times
// during a 'load',
// where N  is the total number of possible versions.
// A version has type uintptr_t.

template <typename T>
class SPMC_Atom
{
public:

    SPMC_Atom()
    {
        if (!writing->version_a.is_lock_free())
            throw std::runtime_error("Not lockfree.");
    }

    // Wait-free

    void store(const T & value)
    {
        ++version;
        writing->version_a = version;
        writing->value = value;
        writing->version_b = version;
        writing = reading.exchange(writing);
    }

    // Lock-free

    T load()
    {
        T value;
        int version_a, version_b;

        do
        {
            Copy * data = reading.load();
            version_b = data->version_b.load();
            value = data->value;
            version_a = data->version_a.load();
        }
        while (version_a != version_b);

        return value;
    }

private:
    struct Copy
    {
        T value;
        atomic<uintptr_t> version_a { 0 };
        atomic<uintptr_t> version_b { 0 };
    };

    int version = 0;

    array<Copy,2> copy;
    Copy * writing { &copy[0] };
    atomic<Copy*> reading { &copy[1] };
};

}
