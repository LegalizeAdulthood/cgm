#include "stream.h"

#include "gkscore.h"
#include "impl.h"

#include <iostream>
#include <cstdlib>

namespace cgm
{
MetafileStreamWriter::MetafileStreamWriter(std::ostream &stream)
    : m_stream(stream),
    m_fd{},
    m_useStream(true)
{
}

MetafileStreamWriter::MetafileStreamWriter(int fd)
    : m_stream(m_buffer),
    m_fd(fd),
    m_useStream(false)
{
}

void MetafileStreamWriter::flushBuffer()
{
    if (m_useStream)
    {
        m_stream.write(m_output, m_outputIndex);
    }
    else
    {
        gks_write_file(m_fd, m_output, m_outputIndex);
    }
    m_outputIndex = 0;
    m_output[0] = 0;
}

}        // namespace cgm
