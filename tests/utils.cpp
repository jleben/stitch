#include "utils.h"
#include "../arguments/arguments.hpp"

#include <regex>

using namespace std;

namespace Testing {

bool Test_Set::run(const Options & options)
{
    string filter_pattern;

    for (auto & pattern : options.filter_regex)
    {
        if (pattern.empty())
            continue;
        if (!filter_pattern.empty())
            filter_pattern += '|';
        filter_pattern += '(';
        filter_pattern += pattern;
        filter_pattern += ')';
    }

    regex filter(filter_pattern);

    vector<string> failed_tests;
    int total_test_count = 0;

    using namespace std;
    for (auto & test : d_tests)
    {
        if (!filter_pattern.empty() && !regex_match(test.first, filter))
        {
            continue;
        }

        ++total_test_count;

        cerr << endl << "-- " << test.first << endl;
        bool ok = test.second();
        cerr << "-- " << (ok ? "PASSED" : "FAILED") << endl;

        if (!ok)
            failed_tests.push_back(test.first);
    }

    if (failed_tests.empty())
    {
        cerr << endl << "All tests passed " << "(" << total_test_count << ")." << endl;
    }
    else
    {
        cerr << endl << "The following tests failed: " << endl;
        for (auto & name : failed_tests)
        {
            cerr << name << endl;
        }
        cerr << "(" << failed_tests.size() << " / " << total_test_count << ")" << endl;
    }

    return (failed_tests.empty() ? 0 : 1);
}

int run(Test_Set & tests, int argc, char * argv[])
{
    Test_Set::Options options;

    Arguments::Parser args;
    args.remaining_arguments(options.filter_regex);
    args.parse(argc, argv);

    return tests.run(options) ? 0 : 1;
}

}
