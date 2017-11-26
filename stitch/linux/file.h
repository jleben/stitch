#pragma once

#include "events.h"
#include "file_event.h"

#include <string>

namespace Stitch {

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

    File(int fd);
    File(const string & path, Access, bool blocking = true);
    ~File();

    File_Event & read_ready() { return d_read_ready; }
    File_Event & write_ready() { return d_write_ready; }

    int read(void * dst, int count);
    int write(void * src, int count);

private:
    static int open_file(const string & path, File::Access access, bool blocking);

    int d_fd;
    File_Event d_read_ready;
    File_Event d_write_ready;
};

}

