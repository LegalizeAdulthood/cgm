#pragma once

#include <cgm/cgm.h>

#include "impl.h"

#include <iosfwd>
#include <sstream>

namespace cgm
{
class MetafileStreamWriter : public MetafileWriter
{
public:
    explicit MetafileStreamWriter(std::ostream &stream);
    explicit MetafileStreamWriter(int fd);

protected:
    virtual void flushBuffer();

    std::ostream &m_stream;
    std::ostringstream m_buffer;
    int m_fd;
    bool m_useStream;
    int m_outputIndex{};             /* output buffer index */
    char m_output[max_buffer + 2]{}; /* output buffer */
};

}        // namespace cgm
