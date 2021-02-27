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

enum class VdcType
{
    Integer = 1,
    Real
};

class MetafileWriter
{
public:
    virtual ~MetafileWriter() = default;

    virtual void beginMetafile(const char *id) = 0;
    virtual void endMetafile() = 0;
    virtual void metafileVersion(int value) = 0;
    virtual void metafileDescription(char const *value) = 0;
    virtual void vdcType(VdcType type) = 0;
    virtual void intPrecisionClearText(int min, int max) = 0;
    virtual void realPrecisionClearText(float minReal, float maxReal, int digits) = 0;
    virtual void indexPrecisionClearText(int min, int max) = 0;
    virtual void colorPrecisionClearText(int max) = 0;
    virtual void colorIndexPrecisionClearText(int max) = 0;
    virtual void maximumColorIndex(int max) = 0;
    virtual void colorValueExtent(int redMin, int redMax, int greenMin, int greenMax, int blueMin, int blueMax) = 0;
};

std::unique_ptr<MetafileWriter> create(std::ostream &stream, Encoding enc);

}
