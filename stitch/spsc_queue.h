#include <atomic>
#include <vector>

namespace Stitch {

using std::vector;
using std::atomic;

// FIXME: Relax memory ordering constraints

/*!
\brief Single-producer-single-consumer queue.

All methods have wait-free progress guarantee.
*/

template <typename T>
class SPSC_Queue
{
public:

    /*! \brief Constructs the queue with the given capacity. */

    SPSC_Queue(int capacity): d_data(capacity + 1) {}

    static bool is_lockfree()
    {
        return ATOMIC_INT_LOCK_FREE;
    }

    /*!
    \brief Maximum number of elements the queue can contain.
    */

    int capacity() const
    {
        return (int) d_data.size()-1;
    }

    /*!
    \brief Whether the queue is full.
    */

    bool full()
    {
        return !writable_size();
    }


    /*!
    \brief Whether the queue is empty.
    */

    bool empty()
    {
        return !readable_size();
    }

    /*!
    \brief Adds an element to the back of the queue.

    The element \p value is added to the back of the queue.
    This can fail if the queue is full, in which case nothing is done.

    \return True on success, false on failure.
    */

    bool push(const T & value)
    {
        if (!writable_size())
            return false;

        d_data[d_write_pos] = value;
        advance_write(1);
        return true;
    }

    /*!
    \brief Adds elements in bulk to the back of the queue.

    Adds \p count consecutive elements starting from the iterator \p input_start,
    which must satisfy the `InputIterator` concept.

    This can fail if the queue is full, in which case nothing is done.

    \return True on success, false on failure.
    */

    template <typename I>
    bool push(int count, I input_start)
    {
        if (writable_size() < count)
            return false;

        int w = d_write_pos;
        int s = d_data.size();
        I input = input_start;

        if (w + count > s)
        {
            int wrap_count = w + count - s;
            for (; w < s; ++w, ++input)
            {
                d_data[w] = *input;
            }
            for (w = 0; w < wrap_count; ++w, ++input)
            {
                d_data[w] = *input;
            }
        }
        else
        {
            int end = w + count;
            for (; w < end; ++w, ++input)
            {
                d_data[w] = *input;
            }
        }

        d_write_pos = w;
        return true;
    }

    /*!
    \brief
    Removes an element from the front of the queue.

    An element is removed from the front of the queue and stored in \p value.

    This can fail if the queue is empty, in which case nothing is done.

    \return True on success, false on failure.
    */

    bool pop(T & value)
    {
        if (!readable_size())
            return false;

        value = d_data[d_read_pos];
        advance_read(1);
        return true;
    }

    /*!
    \brief Removes elements in bulk from the front of the queue.

    Removes \p count elements from the front of the queue and stores
    them in the consecutive positions starting from the iterator \p output_start.
    The iterator must satisfy the `OutputIterator` concept.

    This can fail if the queue is full, in which case nothing is done.

    \return True on success, false on failure.
    */

    template <typename O>
    bool pop(int count, O output_start)
    {
        if (readable_size() < count)
            return false;

        int r = d_read_pos;
        int s = d_data.size();
        O output = output_start;

        if (r + count > s)
        {
            int wrap_count = r + count - s;
            for (; r < s; ++r, ++output)
            {
                *output = d_data[r];
            }
            for (r = 0; r < wrap_count; ++r, ++output)
            {
                *output = d_data[r];
            }
        }
        else
        {
            int end = r + count;
            for (; r < end; ++r, ++output)
            {
                *output = d_data[r];
            }
        }

        d_read_pos = r;
        return true;
    }

private:
    int readable_size() const
    {
        int s = d_data.size();

        if (!s)
            return 0;

        int r = d_read_pos;
        int w = d_write_pos;
        int count = (s + w - r) % s;
        return count;
    }

    int writable_size() const
    {
        int s = d_data.size();

        if (!s)
            return 0;

        int r = d_read_pos;
        int w = d_write_pos;
        int count = (s + r - w - 1) % s;
        return count;
    }

    void advance_write(int count)
    {
        int w = d_write_pos;
        d_write_pos = (d_write_pos + count) % d_data.size();
    }


    void advance_read(int count)
    {
        d_read_pos = (d_read_pos + count) % d_data.size();
    }

    atomic<int> d_write_pos { 0 };
    atomic<int> d_read_pos { 0 };
    vector<T> d_data;
};

}
