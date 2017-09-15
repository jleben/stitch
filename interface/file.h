#pragma once

#include "events.h"

#include <memory>
#include <string>

namespace Concurrency {

using std::string;

class File
{
public:
    enum Access
    {
        ReadOnly,
        WriteOnly,
        ReadWrite
    };

    File(const string & path, Access, bool blocking = true);
    ~File();

    string path() const;

    Event & read_ready();
    Event & write_ready();

    int read(void * dst, int count);
    int write(void * src, int count);

    class Implementation;
private:
    std::shared_ptr<Implementation> d;
};

}


