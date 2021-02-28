#pragma once

#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

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

enum class CharCodeAnnouncer
{
    Basic7Bit = 0,
    Basic8Bit,
    Extended7Bit,
    Extended8Bit
};

enum class ScaleMode
{
    Abstract,
    Metric
};

enum class ColorMode
{
    Indexed,
    Direct
};

enum class LineWidthMode
{
    Absolute,
    Scaled
};

enum class MarkerSizeMode
{
    Absolute,
    Scaled
};

enum class TextFlag
{
    NotFinal,
    Final
};

template <typename T>
struct Point
{
    T x;
    T y;
};

class MetafileWriter
{
public:
    virtual ~MetafileWriter() = default;

    virtual void beginMetafile(const char *id) = 0;
    virtual void endMetafile() = 0;
    virtual void beginPicture(char const *identifier) = 0;
    virtual void beginPictureBody() = 0;
    virtual void endPicture() = 0;
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
    virtual void metafileElementList() = 0;
    virtual void metafileDefaultsReplacement() = 0;
    virtual void fontList(std::vector<std::string> const & fonts) = 0;
    virtual void characterCodingAnnouncer(CharCodeAnnouncer value) = 0;
    virtual void scaleMode(ScaleMode mode, float value) = 0;
    virtual void colorMode(ColorMode mode) = 0;
    virtual void lineWidthMode(LineWidthMode mode) = 0;
    virtual void markerSizeMode(MarkerSizeMode mode) = 0;
    virtual void vdcExtent(int llx, int lly, int urx, int ury) = 0;
    virtual void backgroundColor(int red, int green, int blue) = 0;
    virtual void vdcIntegerPrecision(int min, int max) = 0;
    virtual void clipRectangle(int llx, int lly, int urx, int ury) = 0;
    virtual void clipIndicator(bool enabled) = 0;
    virtual void polyline(const std::vector<Point<int>> &points) = 0;
    virtual void polymarker(const std::vector<Point<int>> &points) = 0;
    virtual void text(Point<int> point, TextFlag flag, const char *text) = 0;
    virtual void polygon(const std::vector<Point<int>> &points) = 0;
    virtual void cellArray(Point<int> c1, Point<int> c2, Point<int> c3, int nx, int ny, int *colors) = 0;
    virtual void lineType(int value) = 0;
    virtual void lineWidth(float value) = 0;
    virtual void lineColor(int value) = 0;
    virtual void markerType(int value) = 0;
    virtual void markerSize(float value) = 0;
    virtual void markerColor(int value) = 0;
    virtual void textFontIndex(int value) = 0;
};

std::unique_ptr<MetafileWriter> create(std::ostream &stream, Encoding enc);

}
