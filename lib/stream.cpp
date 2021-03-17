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
    m_useStream(true),
    m_context{}
{
    m_context.flush_buffer_context = this;
    m_context.flush_buffer = flushBufferCb;

    if (getenv("CGM_SCALE_MODE_METRIC") != nullptr)
        m_context.mm = 0.19685 / max_coord * 1000;
    else
        m_context.mm = 0;
}

MetafileStreamWriter::MetafileStreamWriter(int fd)
    : m_stream(m_buffer),
    m_fd(fd),
    m_useStream(false),
    m_context{}
{
    m_context.flush_buffer_context = this;
    m_context.flush_buffer = flushBufferCb;

    if (getenv("CGM_SCALE_MODE_METRIC") != nullptr)
        m_context.mm = 0.19685 / max_coord * 1000;
    else
        m_context.mm = 0;
}

void MetafileStreamWriter::flushBuffer()
{
    if (m_useStream)
    {
        m_stream.write(m_output, m_context.m_outputIndex);
    }
    else
    {
        gks_write_file(m_fd, m_output, m_context.m_outputIndex);
    }
    m_context.m_outputIndex = 0;
    m_output[0] = 0;
}

void MetafileStreamWriter::flushBufferCb(cgm_context *ctx, void *data)
{
    static_cast<MetafileStreamWriter *>(data)->flushBuffer();
}

}        // namespace cgm
