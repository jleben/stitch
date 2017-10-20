
namespace Reactive {

template <typename T>
class Queue
{
public:
    virtual ~Queue() {}
    virtual bool full() = 0;
    virtual bool empty() = 0;
    virtual bool push(const T &) = 0;
    virtual bool pop(T &) = 0;
};

}
