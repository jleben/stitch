#include "../stitch/connections.h"
#include "../testing/testing.h"

#include <atomic>

using namespace Stitch;
using namespace Testing;
using namespace std;

static bool test_client()
{
    Test test;

    struct Data
    {
        int x = 0;
    };

    Client<Data> a;
    Client<Data> b;
    Client<Data> c;

    {
        int c = 0;
        for(Data & d : a) { ++c; };
        test.assert("Client a has no connections.", c == 0);
    }

    connect(a, b);
    connect(a, c);

    {
        int count = 0;
        for (auto & d : b){ ++count; d.x = 1; };
        test.assert("Client b has one connection.", count == 1);
    }

    {
        int count = 0;
        for (auto & d : c){ ++count; d.x = 2; };
        test.assert("Client c has one connection.", count == 1);
    }

    {
        int count = 0;
        unordered_set<int> values;
        for (auto & d : a){ ++count; values.insert(d.x); };
        test.assert("Client a has two connections.", count == 2);
        test.assert("Client a got value from b.", values.find(1) != values.end());
        test.assert("Client a got value from c.", values.find(2) != values.end());
    }

    return test.success();
}

static bool test_single_server()
{
    struct Data
    {
        int x = 0;
    };

    Test test;

    Client<Data> client1;
    Client<Data> client2;
    Server<Data> server;

    connect(client1, server);
    connect(client2, server);

    {
        int count = 0;
        for(auto & d : client1) { ++count; d.x = 1; };
        test.assert("Client1 has one connection.", count == 1);
    }

    {
        int count = 0;
        for(auto & d : client2) { ++count; d.x = 2; };
        test.assert("Client2 has one connection.", count == 1);
    }

    test.assert("Server has correct data.", server->x == 2);

    server->x = 3;

    for(auto & d : client1)
    {
        test.assert("Client 1 got data from server.", d.x = 3);
    };

    for(auto & d : client2)
    {
        test.assert("Client 2 got data from server.", d.x = 3);
    };

    return test.success();
}

static bool test_multiple_servers()
{
    struct Data
    {
        int x = 0;
    };

    Test test;

    Client<Data> client;
    Server<Data> server1;
    Server<Data> server2;

    connect(client, server1);
    connect(client, server2);

    {
        int count = 0;
        for (auto & d : client) { ++count; d.x = count; };
        test.assert("Client has two connections.", count == 2);
    }

    unordered_set<int> values;
    values.insert(server1->x);
    values.insert(server2->x);

    test.assert("Servers have correct data.",
                values.find(1) != values.end() && values.find(2) != values.end());

    return test.success();
}

static bool test_no_default_constructor()
{
    struct Data
    {
        Data(int v): x(v) {}
        int x;
    };

    Client<Data> client1;
    Client<Data> client2;
    Server<Data> server(make_shared<Data>(3));

    connect(client1, server);
    connect(client1, client2, make_shared<Data>(5));

    return true;
}

Testing::Test_Set connection_tests()
{
    return {
        { "client", test_client },
        { "single-server", test_single_server },
        { "multiple-servers", test_multiple_servers },
        { "no-default-constructor", test_no_default_constructor },
    };
}

