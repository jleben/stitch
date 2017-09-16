#pragma once

#include "events.h"

#include <string>

namespace Reactive {

using std::string;

class File_Event : public Event
{
public:
    enum Type
    {
        Read_Ready,
        Write_Ready
    };

    File_Event(int fd, Type);
    virtual void get_info(int & fd, uint32_t & mode) const override;
    virtual void wait() override;
    virtual void clear() override;

private:
    int d_fd;
    uint32_t d_mode;
};

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

