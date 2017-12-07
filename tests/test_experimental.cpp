#include "../testing/testing.h"

using namespace Testing;

Test_Set variable_tests();

int main(int argc, char * argv[])
{
    Testing::Test_Set tests = {
        { "variable", variable_tests() },
    };

    return Testing::run(tests, argc, argv);
}


