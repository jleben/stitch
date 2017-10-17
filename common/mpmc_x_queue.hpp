#include "queue.hpp"

namespace Reactive {

template <typename T>
class MPMC_X_Queue : public Queue<T>
{
    struct Client
    {
        SPSC_Queue<T> q;
        atomic<bool> dead { false };
    };

public:
    Client * add_producer()
    {
        d_producers.emplace_back();
        return &d_producers.back();
    }

    Client * add_consumer()
    {
        d_consumers.emplace_back();
        return d_consumers;
    }

    void remove_producer(ID id)
    {

    }

    void remove_consumer(ID id)
    {

    }

private:
    void work()
    {
        for(;;)
        {
            for (auto & producer : d_producers)
            {
                while (!producer.q.empty())
                {
                    for (auto & consumer : d_consumers)
                    {
                        // Nope: need a SPMC queue to server consumers
                        consumer.q.push();
                    }
                }
            }
        }
    }

    list<Client> d_producers;
    list<Client> d_consumers;
};

}
