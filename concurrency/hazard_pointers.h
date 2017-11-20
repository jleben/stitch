#pragma once

#include <atomic>
#include <list>
#include <vector>
#include <array>
#include <unordered_set>

namespace Reactive {

using std::atomic;
using std::list;
using std::vector;
using std::array;
using std::unordered_set;

class Hazard_Pointers
{
public:
    static constexpr int K = 2;
    static constexpr int H = 100;

    template<typename T>
    static atomic<T*> & h(int i) { return reinterpret_cast<atomic<T*>&>(d_thread_record.d->h[i]); }

    template <typename T>
    static void reclaim(T * p)
    {
        d_thread_record.d->owned.emplace_back(p);
        cleanup();
    }

private:

    static void cleanup()
    {
        auto & owned = d_thread_record.d->owned;

        if (owned.size() >= H)
        {
            unordered_set<void*> hs;

            for (const auto & record : Hazard_Pointers::d_records)
            {
                for (int i = 0; i < K; ++i)
                {
                    void * h = record.h[i];
                    if (h)
                        hs.insert(h);
                }
            }

            for(auto it = owned.begin(); it != owned.end(); )
            {
                void * p = it->ptr;
                if (hs.find(p) == hs.end())
                    it = owned.erase(it);
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

    struct Private_Record
    {
        std::atomic_flag used { false };
        atomic<void*> h[K];
        list<Owned_Ptr> owned;
    };

    struct Thread_Record
    {
        Thread_Record()
        {
            for (auto & record : Hazard_Pointers::d_records)
            {
                if (!record.used.test_and_set())
                {
                    d = &record;
                    return;
                }
            }

            throw std::runtime_error("Hazard pointers: Too many threads.");
        }

        ~Thread_Record()
        {
            for (int i = 0; i < K; ++i)
            {
                d->h[i] = nullptr;
            }

            d->used.clear();
        }

        Private_Record * d = nullptr;
    };

    thread_local static Thread_Record d_thread_record;

    static array<Private_Record,H> d_records;
};

}
