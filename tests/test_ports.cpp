#include "../concurrency/ports.h"
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

    ClientPort<Data> a;
    ClientPort<Data> b;
    ClientPort<Data> c;

    {
        int c = 0;
        a.for_each([&](Data &){ ++c; });
        test.assert("Client a has no connections.", c == 0);
    }

    connect(a, b);
    connect(a, c);

    {
        int count = 0;
        b.for_each([&](Data & d){ ++count; d.x = 1; });
        test.assert("Client b has one connection.", count == 1);
    }

    {
        int count = 0;
        c.for_each([&](Data & d){ ++count; d.x = 2; });
        test.assert("Client c has one connection.", count == 1);
    }

    {
        int count = 0;
        unordered_set<int> values;
        a.for_each([&](Data & d){ ++count; values.insert(d.x); });
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

    ClientPort<Data> client1;
    ClientPort<Data> client2;
    ServerPort<Data> server;

    connect(client1, server);
    connect(client2, server);

    {
        int count = 0;
        client1.for_each([&](Data & d){ ++count; d.x = 1; });
        test.assert("Client1 has one connection.", count == 1);
    }

    {
        int count = 0;
        client2.for_each([&](Data & d){ ++count; d.x = 2; });
        test.assert("Client2 has one connection.", count == 1);
    }

    test.assert("Server has correct data.", server.data().x == 2);

    server.data().x = 3;

    client1.for_each([&](Data & d){
        test.assert("Client 1 got data from server.", d.x = 3);
    });

    client2.for_each([&](Data & d){
        test.assert("Client 2 got data from server.", d.x = 3);
    });

    return test.success();
}

static bool test_multiple_servers()
{
    struct Data
    {
        int x = 0;
    };

    Test test;

    ClientPort<Data> client;
    ServerPort<Data> server1;
    ServerPort<Data> server2;

    connect(client, server1);
    connect(client, server2);

    {
        int count = 0;
        client.for_each([&](Data & d){ ++count; d.x = count; });
        test.assert("Client has two connections.", count == 2);
    }

    unordered_set<int> values;
    values.insert(server1.data().x);
    values.insert(server2.data().x);

    test.assert("Servers have correct data.",
                values.find(1) != values.end() && values.find(2) != values.end());

    return test.success();
}

Testing::Test_Set port_tests()
{
    return {
        { "client", test_client },
        { "single-server", test_single_server },
        { "multiple-servers", test_multiple_servers },
    };
}

