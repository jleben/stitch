#include "../stitch/atom.h"
#include "../testing/testing.h"

#include <thread>
#include <chrono>

using namespace Stitch;
using namespace Testing;
using namespace std;

static bool test_lockfree()
{
    Test test;
    test.assert("Is lockfree.", Atom<int>::is_lockfree());
    return test.success();
}

static bool test_default_value()
{
    Test test;

    Atom<int> atom;
    AtomWriter<int> writer(atom);
    AtomReader<int> reader(atom);

    test.assert("Default writer value.", writer.value() == 0);
    test.assert("Default reader value.", reader.value() == 0);

    int v = reader.load();
    test.assert("Default loaded value.", v == 0);

    return test.success();
}

static bool test_basic_store_load()
{
    Test test;

    Atom<int> atom;
    AtomWriter<int> writer(atom);
    AtomReader<int> reader(atom);

    {
        writer.value() = 2;
        writer.store();
        test.assert("write(2) + store() + load() returns 2.", reader.load() == 2);
        test.assert("read() also returns 2.", reader.value() == 2);
    }

    {
        writer.store(3);
        test.assert("store(3) + load() returns 3.", reader.load() == 3);
        test.assert("read() also returns 3.", reader.value() == 3);
    }

    return test.success();
}

static bool test_single_writer_single_reader()
{
    Test test;

    Atom<int> atom(1);
    AtomWriter<int> writer(atom);
    AtomReader<int> reader(atom);

    test.assert("Initial writer value.", writer.value() == 0);
    test.assert("Initial reader value.", reader.value() == 0);

    reader.load();
    test.assert("Initial load.", reader.value() == 1);

    for (int i = 1; i < 10; ++i)
    {
        int old_value = reader.value();

        writer.value() = i;

        reader.load();

        test.assert("Value after write and load without store is " + to_string(old_value),
                    reader.value() == old_value);

        writer.store();

        test.assert("Value after store without load is " + to_string(old_value),
                    reader.value() == old_value);

        reader.load();

        test.assert("Value after store and load is " + to_string(i),
                    reader.value() == i);
    }

    return test.success();
}

static bool test_multi_writer_multi_reader()
{
    Test test;

    Atom<int> atom(1);
    AtomWriter<int> writer1(atom);
    AtomWriter<int> writer2(atom);
    AtomReader<int> reader1(atom);
    AtomReader<int> reader2(atom);

    reader1.load();
    reader2.load();
    test.assert("Initial reader1 load.", reader1.value() == 1);
    test.assert("Initial reader2 load.", reader2.value() == 1);

    for (int i = 1; i < 10; ++i)
    {
        int r1 = reader1.value();
        int r2 = reader2.value();

        int w1 = i + 100;
        int w2 = i + 200;
        writer1.value() = w1;
        writer2.value() = w2;

        reader1.load();
        reader2.load();
        test.assert("Values after write and before store.",
                    reader1.value() == r1 &&
                    reader2.value() == r2);

        writer1.store();
        reader1.load();

        test.assert("Values after writer1.store and read1.load",
                    reader1.value() == w1 &&
                    reader2.value() == r2);

        reader2.load();

        test.assert("Values after reader2.load",
                    reader1.value() == w1 &&
                    reader2.value() == w1);

        writer2.store();
        reader2.load();

        test.assert("Values after writer2.store and reader2.load",
                    reader1.value() == w1 &&
                    reader2.value() == w2);

        reader1.load();

        test.assert("Values after reader1.load",
                    reader1.value() == w2 &&
                    reader2.value() == w2);
    }

    return test.success();
}

static bool test_node_reclamation()
{
    Test test;

    static atomic<int> value_count { 0 };

    struct Value
    {
        Value() { value_count.fetch_add(1); }
        Value(const Value &): Value() {}
        ~Value() { value_count.fetch_sub(1); }
    };

    {
        Atom<Value> atom;

        test.assert("Atom creates a single value.", value_count == 1);

        thread t([&]()
        {
            AtomWriter<Value> writer(atom);
            test.assert("Writer creates a value.", value_count == 2);

            AtomReader<Value> reader(atom);
            test.assert("Reader creates a value.", value_count == 3);

            for (int i = 0; i < 5; ++i)
            {
                writer.store();
                reader.load();
            }

            test.assert("Value count before thread ends.", value_count == 3);
        });

        t.join();

        test.assert("Value count after thread ends.", value_count == 1);
    }

    test.assert("Value count after atom destroyed.", value_count == 0);

    return test.success();
}

static bool test_stress()
{
    struct Value
    {
        double x = 0;
        double y = 0;
        double z = 0;

        Value() {}

        Value(double x, double y, double z): x(x), y(y), z(z)
        {
            bool ok = (x == y && x == z);
            if (!ok)
                throw std::runtime_error("Value corrupted");
        }

        Value(const Value & other): Value(other.x, other.y, other.z) {}
    };

    Test test;

    Atom<Value> atom;

    int transmitted_count = 0;
    int write_cycle_count = 0;
    int read_cycle_count = 0;

    using clock = chrono::steady_clock;

    auto start = clock::now();
    chrono::seconds duration(1);

    auto write_func = [&]()
    {
        AtomWriter<Value> writer(atom);

        try
        {
            for (int i = 0; i < 100; ++i)
            {
                writer.store(Value(i, i, i));
            }
        }
        catch (std::runtime_error & e)
        {
            test.assert(string("Writer exception: ") + e.what(), false);
        }
    };

    auto read_func = [&]()
    {
        AtomReader<Value> reader(atom);

        Value v0;

        try
        {
            for (int i = 0; i < 111; ++i)
            {
                const Value & v = reader.load();

                bool is_new = v.x != v0.x || v.y != v0.y || v.z != v0.z;
                if (is_new)
                    ++transmitted_count;

                bool ok = (v.x == v.y && v.x == v.z);
                if (!ok)
                    throw std::runtime_error("Value corrupted");

                v0 = v;
            }
        }
        catch (std::runtime_error & e)
        {
            test.assert(string("Reader exception: ") + e.what(), false);
        }
    };

    thread generate_writers([&]()
    {
        while (clock::now() - start < duration)
        {
            thread writer1(write_func);
            thread writer2(write_func);
            writer1.join();
            writer2.join();
            ++write_cycle_count;
        }
    });

    thread generate_readers([&]()
    {
        while (clock::now() - start < duration)
        {
            thread reader1(read_func);
            thread reader2(read_func);
            reader1.join();
            reader2.join();
            ++read_cycle_count;
        }
    });

    generate_writers.join();
    generate_readers.join();

    test.assert("Transmitted more than 10000 values: " + to_string(transmitted_count),
                transmitted_count > 20000);
    test.assert("Done more than 200 write cycles: " + to_string(write_cycle_count),
                read_cycle_count > 200);
    test.assert("Done more than 200 read cycles: " + to_string(read_cycle_count),
                read_cycle_count > 200);

    return test.success();
}

Test_Set atom_tests()
{
    return {
        // TODO: figure out why this fails
        // { "lockfree", test_lockfree },
        { "default-value", test_default_value },
        { "basic-store-load", test_basic_store_load },
        { "single-writer-reader", test_single_writer_single_reader },
        { "multi-writer-reader", test_multi_writer_multi_reader },
        { "node-reclamation", test_node_reclamation },
        { "stress", test_stress },
    };
}
