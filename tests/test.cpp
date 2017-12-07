#include "../testing/testing.h"

using namespace Testing;

Test_Set waitfree_spsc_queue_tests();
Test_Set waitfree_mpsc_queue_tests();
Test_Set waitfree_mpmc_queue_tests();
Test_Set lockfree_mpmc_queue_tests();
Test_Set lockfree_set_tests();
Test_Set spmc_atom_tests();
Test_Set stream_tests();
Test_Set variable_tests();
Test_Set connection_tests();
Test_Set signal_tests();
Test_Set timer_tests();
Test_Set file_tests();
Test_Set event_reactor_tests();

int main(int argc, char * argv[])
{
    Testing::Test_Set tests = {
        { "waitfree-spsc-queue", waitfree_spsc_queue_tests() },
        { "waitfree-mpsc-queue", waitfree_mpsc_queue_tests() },
        { "waitfree-mpmc-queue", waitfree_mpmc_queue_tests() },
        { "lockfree-mpmc-queue", lockfree_mpmc_queue_tests() },
        { "lockfree-set", lockfree_set_tests() },
        { "spmc-atom", spmc_atom_tests() },
        { "connections", connection_tests() },
        { "stream", stream_tests() },
        { "variable", variable_tests() },
        { "signal", signal_tests() },
        { "timer", timer_tests() },
        { "file", file_tests() },
        { "event-reactor", event_reactor_tests() },
    };

    return Testing::run(tests, argc, argv);
}
