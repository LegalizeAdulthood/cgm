#pragma once

#include <iosfwd>
#include <memory>

namespace cgm
{

enum class Encoding
{
    Binary = 0,
    Character = 1,
    ClearText = 2
};

class MetafileWriter;

class MetafileOutputWorkstation
{
public:
    virtual ~MetafileOutputWorkstation() = default;
};

std::unique_ptr<MetafileOutputWorkstation> create(std::unique_ptr<MetafileWriter> writer, Encoding encoding);

}
