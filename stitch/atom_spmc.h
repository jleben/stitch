#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <type_traits>

namespace Stitch {

using std::atomic;
using std::array;

/*!
\brief Lock-free atomically updated container of a single value of type T.

Type T must be trivially copyable.

This class uses versioning to detect thread contention.
It will operate correctly
as long as \ref store is executed less than N times
during a single \ref load,
where N  is the total number of possible versions.
A version has type uintptr_t.
*/

template <typename T>
class SPMC_Atom
{
public:

    /*!
     * \brief Constructs the container with a default-constructed value.
     *
     * Throws a std::runtime_error if type T is not a trivially copyable type.
     */
    SPMC_Atom()
    {
        if (!writing->version_a.is_lock_free())
            throw std::runtime_error("Not lockfree.");
        if (!std::is_trivially_copyable<T>::value)
            throw std::runtime_error("Value type is not trivial.");
    }

    /*!
     * \brief Constructs the container with a given value.
     */
    SPMC_Atom(const T & value)
    {
        if (!writing->version_a.is_lock_free())
            throw std::runtime_error("Not lockfree.");

        reading.load()->value = value;
    }

    // Wait-free

    /*!
    \brief Store `value` in the container.

    Progress: Lock-free.
    */
    void store(const T & value)
    {
        ++version;
        writing->version_a = version;
        writing->value = value;
        writing->version_b = version;
        writing = reading.exchange(writing);
    }

    // Lock-free

    /*!
    \brief Load the last value stored in the container.

    Progress: Lock-free.
    */
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
