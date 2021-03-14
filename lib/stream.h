#pragma once

#include <cgm/cgm.h>

#include "context.h"

#include <iosfwd>

namespace cgm
{
class MetafileStreamWriter : public MetafileWriter
{
public:
    MetafileStreamWriter(std::ostream &stream);

protected:
    std::ostream &m_stream;
    cgm_context m_context;

private:
    void flushBuffer();
    static void flushBufferCb(cgm_context *ctx, void *data);
};

}        // namespace cgm
