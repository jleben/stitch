#pragma once

#include <functional>
#include <vector>

namespace Reactive {

using std::function;
using std::vector;

template <typename T>
class Stream_Generator
{
public:
    using Callback = function<void(T)>;

    template <typename F>
    void subscribe(F f)
    {
        callbacks.push_back(f);
    }

    void push(const T & value)
    {
        for (auto & cb : callbacks)
            cb(value);
    }

private:
    vector<Callback> callbacks;
};

}
