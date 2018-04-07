#include "../stitch/atom.h"
#include "../testing/testing.h"

#include <thread>

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

static bool test_nontrivial_value()
{
    Test test;

    static atomic<int> value_count { 0 };

    struct Value
    {
        double x = 1.1;
        double y = 2.2;

        Value(): Value(1.1, 2.2) {  }
        Value(double x, double y): x(x), y(y) { value_count.fetch_add(1); }
        Value(const Value & other): Value(other.x, other.y) {}

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

Test_Set atom_tests()
{
    return {
        { "lockfree", test_lockfree },
        { "default-value", test_default_value },
        { "single-writer-reader", test_single_writer_single_reader },
        { "multi-writer-reader", test_multi_writer_multi_reader },
        { "non-trivial-value", test_nontrivial_value },
    };
}
