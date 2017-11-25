#pragma once

#include "lockfree_set.h"

#include <memory>

namespace Reactive {

using std::shared_ptr;

namespace Detail {

template <typename T>
struct PortData;

template <typename T>
using PortPtr = shared_ptr<PortData<T>>;

template <typename T>
struct Link
{
    shared_ptr<PortData<T>> peer;
    shared_ptr<T> data;
};

template <typename T>
using LinkPtr = shared_ptr<Link<T>>;


template <typename T>
struct PortData
{
    LinkPtr<T> find_link(PortPtr<T> peer)
    {
        for (const auto & link : links)
        {
            if (link->peer == peer)
                return link;
        }

        return nullptr;
    }

    Set<LinkPtr<T>> links;
};

}

template <typename T> class ClientPort;
template <typename T> class ServerPort;

template <typename T>
void connect(ClientPort<T> & client, ServerPort<T> & server);

template <typename T>
void disconnect(ClientPort<T> & client, ServerPort<T> & server);

template <typename T>
void connect(ClientPort<T> & client, ClientPort<T> & server);

template <typename T>
void disconnect(ClientPort<T> & client, ClientPort<T> & server);

template <typename T>
class ClientPort
{
public:
    friend void connect<T>(ClientPort<T> & client, ClientPort<T> & server);
    friend void disconnect<T>(ClientPort<T> & client, ClientPort<T> & server);
    friend void connect<T>(ClientPort<T> & client, ServerPort<T> & server);
    friend void disconnect<T>(ClientPort<T> & client, ServerPort<T> & server);

    ClientPort(): p(std::make_shared<Detail::PortData<T>>()) {}

    ~ClientPort()
    {
        for(const auto & link : p->links)
        {
            auto peer = link->peer;
            auto peer_link = peer->find_link(p);
            if (peer_link)
            {
                peer->links.remove(peer_link);
            }
        };
    }

    template <typename F>
    void for_each(F f)
    {
        for(const auto & link : p->links)
        {
            f(*link->data);
        }
    }

private:
    shared_ptr<Detail::PortData<T>> p;
};

template <typename T>
class ServerPort
{
public:
    friend void connect<T>(ClientPort<T> & client, ServerPort<T> & server);
    friend void disconnect<T>(ClientPort<T> & client, ServerPort<T> & server);

    ServerPort():
        p(std::make_shared<Detail::PortData<T>>()),
        d(std::make_shared<T>())
    {}

    ~ServerPort()
    {
        for(const auto & link : p->links)
        {
            auto peer = link->peer;
            auto link2 = peer->find_link(p);
            if (link2)
                peer->links.remove(link2);
        };
    }

    T & data()
    {
        return *d;
    }

private:
    shared_ptr<Detail::PortData<T>> p;
    shared_ptr<T> d;
};

template <typename T>
void connect(ClientPort<T> & client, ServerPort<T> & server)
{
    {
        auto link = std::make_shared<Detail::Link<T>>();
        link->peer = server.p;
        link->data = server.d;
        client.p->links.insert(link);
    }
    {
        auto link = std::make_shared<Detail::Link<T>>();
        link->peer = client.p;
        server.p->links.insert(link);
    }
}

template <typename T>
void disconnect(ClientPort<T> & client, ServerPort<T> & server)
{
    {
        auto link = client.p->find_link(server.p);
        if (link)
            client.p->links.remove(link);
    }
    {
        auto link = server.p->find_link(client.p);
        if (link)
            server.p->links.remove(link);
    }
}

template <typename T>
void connect(ClientPort<T> & client1, ClientPort<T> & client2)
{
    if (&client1 == &client2)
        return;

    auto data = std::make_shared<T>();

    {
        auto link = std::make_shared<Detail::Link<T>>();
        link->peer = client2.p;
        link->data = data;
        client1.p->links.insert(link);
    }
    {
        auto link = std::make_shared<Detail::Link<T>>();
        link->peer = client1.p;
        link->data = data;
        client2.p->links.insert(link);
    }
}

template <typename T>
void disconnect(ClientPort<T> & client1, ClientPort<T> & client2)
{
    {
        auto link = client1.p->find_link(client2.p);
        if (link)
            client2.p->links.remove(link);
    }
    {
        auto link = client2.p->find_link(client1.p);
        if (link)
            client2.p->links.remove(link);
    }
}

}
