#pragma once

#include <cgm/cgm.h>

#include "context.h"

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
    std::ostream &m_stream;
    std::ostringstream m_buffer;
    int m_fd;
    bool m_useStream;
    int m_outputIndex{};             /* output buffer index */
    char m_output[max_buffer + 2]{}; /* output buffer */
    void *m_flushBufferCtx{};
    void (*m_flushBuffer)(cgm_context *p, void *data){};
    cgm_context m_context;

private:
    void flushBuffer();
    static void flushBufferCb(cgm_context *ctx, void *data);
};

}        // namespace cgm
