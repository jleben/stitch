#pragma once

#include <atomic>
#include <list>
#include <vector>
#include <array>
#include <unordered_set>

namespace Stitch {
namespace Detail {

using std::atomic;
using std::list;
using std::vector;
using std::array;
using std::unordered_set;

template <typename T>
class Hazard_Pointer
{
public:
    atomic<T*> pointer;
    bool acquire() { return !used.test_and_set(); }
    void release() { used.clear(); }
private:
    std::atomic_flag used;
};

class Hazard_Pointers
{
public:
    // NOTE: H must be power of two
    static constexpr int H = 256;

    template<typename T> static
    Hazard_Pointer<T> & acquire()
    {
        int i = d_pointer_alloc_hint.load();
        int j = i;
        int m = H-1;
        int c = 0;
        do
        {
            ++c;
            j = (j + 1) & m;
            if (d_pointers[j].acquire())
            {
                d_pointer_alloc_hint = j;
                return reinterpret_cast<Hazard_Pointer<T>&>(d_pointers[j]);
            }
        }
        while (j != i);

        //if (c > 1)
            //printf("Retries: %d\n", c);

        throw std::runtime_error("Ran out of pointers.");
    }

    template <typename T>
    static void reclaim(T * p)
    {
        d_thread_record.owned.emplace_back(p);
        cleanup();
    }

private:

    static void cleanup()
    {
        auto & owned = d_thread_record.owned;

        if (owned.size() >= H)
        {
            unordered_set<void*> hs;

            for (const auto & pointer : Hazard_Pointers::d_pointers)
            {
                void * h = pointer.pointer;
                if (h)
                    hs.insert(h);
            }

            for(auto it = owned.begin(); it != owned.end(); )
            {
                void * p = it->ptr;
                if (hs.find(p) == hs.end())
                {
                    //printf("Deleting %p\n", p);
                    it = owned.erase(it);
                }
                else
                    ++it;
            }
        }
    }

    template <typename T>
    struct Deleter
    {
        static void del(void *p) { delete (T*)p; }
    };

    struct Owned_Ptr
    {
        template <typename T>
        Owned_Ptr(T * ptr): ptr(ptr), deleter(&Deleter<T>::del) {}
        Owned_Ptr(const Owned_Ptr &) = delete;
        Owned_Ptr & operator=(const Owned_Ptr &) = delete;
        ~Owned_Ptr() { deleter(ptr); }
        void * ptr;
        void (*deleter)(void *);
    };

    struct Thread_Record
    {
        list<Owned_Ptr> owned;
    };

    thread_local static Thread_Record d_thread_record;

    static atomic<int> d_pointer_alloc_hint;
    static array<Hazard_Pointer<void>,H> d_pointers;
};

}
}
