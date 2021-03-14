#include <iostream>
#include <memory>
#include <stdexcept>

#include <cgm/cgm.h>

#include "binary.h"
#include "clear_text.h"

namespace cgm
{

std::unique_ptr<MetafileWriter> create(std::ostream &stream, Encoding enc)
{
    if (enc == Encoding::Binary)
    {
        return std::make_unique<BinaryMetafileWriter>(stream);
    }
    if (enc == Encoding::ClearText)
    {
        return std::make_unique<ClearTextMetafileWriter>(stream);
    }

    throw std::runtime_error("Unsupported encoding");
}

}        // namespace cgm
