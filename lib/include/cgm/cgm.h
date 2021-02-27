#pragma once

#include <iosfwd>
#include <memory>

#include <stdio.h>

namespace cgm
{

enum class Encoding
{
    Binary = 7,
    ClearText = 8,
    Character = 9
};

void beginMetafile(FILE *file, Encoding enc);
void endMetafile(FILE *file);

class MetafileWriter
{
public:
    virtual ~MetafileWriter() = default;

    virtual void beginMetafile(const char *id) = 0;
    virtual void endMetafile() = 0;
    virtual void metafileVersion(int value) = 0;
};

std::unique_ptr<MetafileWriter> create(std::ostream &stream, Encoding enc);

}
