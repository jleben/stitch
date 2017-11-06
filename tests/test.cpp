#include "../testing/testing.h"

using namespace Testing;

Test_Set spsc_queue_tests();
Test_Set mpsc_queue_tests();
Test_Set mpmc_queue_tests();
Test_Set stream_tests();
Test_Set signal_tests();
Test_Set timer_tests();
Test_Set file_tests();

int main(int argc, char * argv[])
{
    Testing::Test_Set tests = {
        { "spsc-queue", spsc_queue_tests() },
        { "mpsc-queue", mpsc_queue_tests() },
        { "mpmc-queue", mpmc_queue_tests() },
        { "stream", stream_tests() },
        { "signal", signal_tests() },
        { "timer", timer_tests() },
        { "file", file_tests() },
    };

    return Testing::run(tests, argc, argv);
}
