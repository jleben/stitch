
namespace Reactive {

template <typename T>
class Queue
{
public:
    virtual ~Queue() {}
    virtual bool empty() = 0;
    virtual void push(const T &) = 0;
    virtual T pop() = 0;
};

}
