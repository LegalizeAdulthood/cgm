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
    Integer = 0,
    Real
};

enum class CharCodeAnnouncer
{
    Basic7Bit = 0,
    Basic8Bit,
    Extended7Bit,
    Extended8Bit
};

enum class ScalingMode
{
    Abstract,
    Metric
};

enum class ColorMode
{
    Indexed,
    Direct
};

enum class SpecificationMode
{
    Absolute,
    Scaled
};

enum class TextFlag
{
    NotFinal,
    Final
};

enum class TextPrecision
{
    String,
    Character,
    Stroke
};

enum class TextPath
{
    Right,
    Left,
    Up,
    Down
};

enum class HorizAlign
{
    Normal,
    Left,
    Center,
    Right,
    Continuous
};

enum class VertAlign
{
    Normal,
    Top,
    Cap,
    Half,
    Base,
    Bottom,
    Continuous
};

enum class InteriorStyle
{
    Hollow,
    Solid,
    Pattern,
    Hatch,
    Empty
};

enum class RealPrecision
{
    Floating,
    Fixed
};

// Predefined line types.
enum LineType
{
    LT_Solid = 1,
    LT_Dash = 2,
    LT_Dot = 3,
    LT_DashDot = 4,
    LT_DashDotDot = 5
};

// Predefined marker types.
enum MarkerType
{
    MT_Dot = 1,
    MT_Plus = 2,
    MT_Asterisk = 3,
    MT_Circle = 4,
    MT_Cross = 5
};

// Predefined hatch types.
enum HatchType
{
    HT_HorizontalLines = 1,
    HT_VerticalLines = 2,
    HT_PositiveSlopeLines = 3,
    HT_NegativeSlopeLines = 4,
    HT_HorizontalVerticalCrossHatch = 5,
    HT_PositiveNegativeSlopeCrossHatch = 6
};

template <typename T>
struct Point
{
    T x;
    T y;
};

struct Color
{
    float red;
    float green;
    float blue;
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
    virtual void intPrecisionBinary(int value) = 0;
    virtual void realPrecisionClearText(float minReal, float maxReal, int digits) = 0;
    virtual void realPrecisionBinary(RealPrecision prec, int expWidth, int mantWidth) = 0;
    virtual void indexPrecisionClearText(int min, int max) = 0;
    virtual void indexPrecisionBinary(int value) = 0;
    virtual void colorPrecisionClearText(int max) = 0;
    virtual void colorPrecisionBinary(int value) = 0;
    virtual void colorIndexPrecisionClearText(int max) = 0;
    virtual void colorIndexPrecisionBinary(int value) = 0;
    virtual void maximumColorIndex(int max) = 0;
    virtual void colorValueExtent(int redMin, int redMax, int greenMin, int greenMax, int blueMin, int blueMax) = 0;
    virtual void metafileElementList() = 0;
    virtual void fontList(std::vector<std::string> const & fonts) = 0;
    virtual void characterCodingAnnouncer(CharCodeAnnouncer value) = 0;
    virtual void scalingMode(ScalingMode mode, float value) = 0;
    virtual void colorSelectionMode(ColorMode mode) = 0;
    virtual void lineWidthSpecificationMode(SpecificationMode mode) = 0;
    virtual void markerSizeSpecificationMode(SpecificationMode mode) = 0;
    virtual void vdcExtent(int llx, int lly, int urx, int ury) = 0;
    virtual void backgroundColor(int red, int green, int blue) = 0;
    virtual void vdcIntegerPrecisionClearText(int min, int max) = 0;
    virtual void vdcIntegerPrecisionBinary(int value) = 0;
    virtual void clipRectangle(int llx, int lly, int urx, int ury) = 0;
    virtual void clipIndicator(bool enabled) = 0;
    virtual void polyline(const std::vector<Point<int>> &points) = 0;
    virtual void polymarker(const std::vector<Point<int>> &points) = 0;
    virtual void text(Point<int> point, TextFlag flag, const char *text) = 0;
    virtual void polygon(const std::vector<Point<int>> &points) = 0;
    virtual void cellArray(Point<int> c1, Point<int> c2, Point<int> c3, int colorPrecision, int nx, int ny, const int *colors) = 0;
    virtual void lineType(int value) = 0;
    virtual void lineWidth(float value) = 0;
    virtual void lineColor(int value) = 0;
    virtual void markerType(int value) = 0;
    virtual void markerSize(float value) = 0;
    virtual void markerColor(int value) = 0;
    virtual void textFontIndex(int value) = 0;
    virtual void textPrecision(TextPrecision value) = 0;
    virtual void charExpansion(float value) = 0;
    virtual void charSpacing(float value) = 0;
    virtual void textColor(int index) = 0;
    virtual void charHeight(int value) = 0;
    virtual void charOrientation(int upX, int upY, int baseX, int baseY) = 0;
    virtual void textPath(TextPath value) = 0;
    virtual void textAlignment(HorizAlign horiz, VertAlign vert, float contHoriz, float contVert) = 0;
    virtual void interiorStyle(InteriorStyle value) = 0;
    virtual void fillColor(int value) = 0;
    virtual void hatchIndex(int value) = 0;
    virtual void patternIndex(int value) = 0;
    virtual void colorTable(int startIndex, std::vector<Color> const &colors) = 0;
};

std::unique_ptr<MetafileWriter> create(std::ostream &stream, Encoding enc);

}
