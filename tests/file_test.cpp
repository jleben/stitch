#include "../linux/file.h"
#include "../testing/testing.h"

#include <thread>
#include <chrono>
#include <iostream>
#include <sstream>
#include <cstring>

#include <sys/types.h>
#include <sys/stat.h>

using namespace Reactive;
using namespace std;
using namespace Testing;

bool test_basic()
{
    Test test;

    const string path("test.fifo");

    {
        remove(path.c_str());

        int result = mkfifo(path.c_str(), S_IWUSR | S_IRUSR);
        if (result == -1)
        {
            throw std::runtime_error(string("Failed to create FIFO: ") + strerror(errno));
        }
    }

    int reps = 5;

    auto make_data = [](int rep) -> string
    {
        ostringstream s;
        s << "data";

        for(int i = 0; i < rep; ++i)
            s << '.' << to_string(i+1);

        return s.str();
    };

    thread writer([&]()
    {
        File f(path, File::WriteOnly);

        uint32_t d = 0;

        for(int rep = 0; rep < reps; ++rep)
        {
            f.write_ready().wait();

            printf("Write ready\n");

            ++d;

            int c = f.write(&d, sizeof(d));

            printf("Written %d\n", d);

            if (c != sizeof(d))
                throw std::runtime_error("Failed write.");

            this_thread::sleep_for(chrono::milliseconds(250));
        }
    });

    thread reader([&]()
    {
        File f(path, File::ReadOnly);

        uint32_t expected = 0;

        for(int rep = 0; rep < reps; ++rep)
        {
            f.read_ready().wait();

            printf("Read ready\n");

            ++expected;

            uint32_t received = 0;

            int c = f.read(&received, sizeof(uint32_t));

            printf("Read %d\n", received);

            if (c != sizeof(uint32_t))
                throw std::runtime_error("Failed read.");

            test.assert("Received = " + to_string(expected), received == expected);
        }
    });

    writer.join();
    reader.join();

    return test.success();
}

bool test_blocking_read()
{
    Test test;

    const string path("test.fifo");

    {
        remove(path.c_str());

        int result = mkfifo(path.c_str(), S_IWUSR | S_IRUSR);
        if (result == -1)
        {
            throw std::runtime_error(string("Failed to create FIFO: ") + strerror(errno));
        }
    }

    int reps = 5;

    auto make_data = [](int rep) -> string
    {
        ostringstream s;
        s << "data";

        for(int i = 0; i < rep; ++i)
            s << '.' << to_string(i+1);

        return s.str();
    };

    thread writer([&]()
    {
        File f(path, File::WriteOnly);

        uint32_t d = 0;

        for(int rep = 0; rep < reps; ++rep)
        {
            f.write_ready().wait();

            printf("Write ready\n");

            ++d;

            int c = f.write(&d, sizeof(d));

            printf("Written %d\n", d);

            if (c != sizeof(d))
                throw std::runtime_error("Failed write.");

            this_thread::sleep_for(chrono::milliseconds(250));
        }
    });

    thread reader([&]()
    {
        File f(path, File::ReadOnly);

        vector<uint32_t> received(reps, 0);

        f.read_ready().wait();

        printf("Read ready\n");

        int c = f.read(&received[0], sizeof(uint32_t) * reps);

        printf("Read count = %d\n", c);

        if (c != sizeof(uint32_t) * reps)
            throw std::runtime_error("Failed read.");

        for(int rep = 0; rep < reps; ++rep)
        {
            auto & v = received[rep];

            test.assert("Received = " + to_string(v), v == rep + 1);
        }
    });

    writer.join();
    reader.join();

    return test.success();
}

bool test_nonblocking_read()
{
    Test test;

    const string path("test.fifo");

    {
        remove(path.c_str());

        int result = mkfifo(path.c_str(), S_IWUSR | S_IRUSR);
        if (result == -1)
        {
            throw std::runtime_error(string("Failed to create FIFO: ") + strerror(errno));
        }
    }

    int reps = 5;

    auto make_data = [](int rep) -> string
    {
        ostringstream s;
        s << "data";

        for(int i = 0; i < rep; ++i)
            s << '.' << to_string(i+1);

        return s.str();
    };

    thread writer([&]()
    {
        File f(path, File::WriteOnly);

        uint32_t d = 0;

        for(int rep = 0; rep < reps; ++rep)
        {
            f.write_ready().wait();

            printf("Write ready\n");

            ++d;

            int c = f.write(&d, sizeof(d));

            printf("Written %d\n", d);

            if (c != sizeof(d))
                throw std::runtime_error("Failed write.");

            this_thread::sleep_for(chrono::milliseconds(250));
        }
    });

    thread reader([&]()
    {
        File f(path, File::ReadOnly, false);

        vector<uint32_t> received(reps, 0);

        int total_size = sizeof(uint32_t) * reps;
        int received_size = 0;
        char * dst = (char*) &received[0];

        while(received_size < total_size)
        {
            f.read_ready().wait();

            printf("Read ready\n");

            int c = f.read(dst, total_size - received_size);

            if (c <= 0)
                throw std::runtime_error("Failed read.");

            printf("Read count = %d\n", c);

            received_size += c;
            dst += c;
        }

        for(int rep = 0; rep < reps; ++rep)
        {
            auto & v = received[rep];

            assert("Received = " + to_string(v), v == rep + 1);
        }
    });

    writer.join();
    reader.join();

    return test.success();
}

int main(int argc, char * argv[])
{
    Test_Set t = {
        { "basic", test_basic },
        { "blocking read", test_blocking_read },
        { "nonblocking read", test_nonblocking_read },
    };

    return Testing::run(t, argc, argv);
}
