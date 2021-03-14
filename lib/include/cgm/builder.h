#pragma once

#include <cgm/cgm.h>

#include <memory>

namespace cgm
{

class PictureDescriptorBuilder;

class NewPictureBuilder
{
public:
    NewPictureBuilder(std::unique_ptr<MetafileWriter> &&writer)
        : m_writer(std::move(writer))
    {
    }

    PictureDescriptorBuilder beginPicture(const char *identifier);

    std::unique_ptr<MetafileWriter> endMetafile()
    {
        m_writer->endMetafile();
        return std::move(m_writer);
    }

private:
    std::unique_ptr<MetafileWriter> m_writer;
};

class PictureBodyBuilder
{
    using Self = PictureBodyBuilder;

public:
    PictureBodyBuilder(std::unique_ptr<MetafileWriter> &&writer)
        : m_writer(std::move(writer))
    {
    }

    Self &vdcIntegerPrecision(int min, int max)
    {
        m_writer->vdcIntegerPrecisionClearText(min, max);
        return *this;
    }
    Self &vdcIntegerPrecision(int value)
    {
        m_writer->vdcIntegerPrecisionBinary(value);
        return *this;
    }
    Self &clipRectangle(int llx, int lly, int urx, int ury)
    {
        m_writer->clipRectangle(llx, lly, urx, ury);
        return *this;
    }
    Self &clipIndicator(bool enabled)
    {
        m_writer->clipIndicator(enabled);
        return *this;
    }
    Self &polyline(const std::vector<Point<int>> &points)
    {
        m_writer->polyline(points);
        return *this;
    }
    Self &polymarker(const std::vector<Point<int>> &points)
    {
        m_writer->polymarker(points);
        return *this;
    }
    Self &text(Point<int> point, TextFlag flag, const char *text)
    {
        m_writer->text(point, flag, text);
        return *this;
    }
    Self &polygon(const std::vector<Point<int>> &points)
    {
        m_writer->polygon(points);
        return *this;
    }
    Self &cellArray(Point<int> c1, Point<int> c2, Point<int> c3, int colorPrecision, int nx, int ny, int *colors)
    {
        m_writer->cellArray(c1, c2, c3, colorPrecision, nx, ny, colors);
        return *this;
    }
    Self &lineType(int value)
    {
        m_writer->lineType(value);
        return *this;
    }
    Self &lineWidth(float value)
    {
        m_writer->lineWidth(value);
        return *this;
    }
    Self &lineColor(int value)
    {
        m_writer->lineColor(value);
        return *this;
    }
    Self &markerType(int value)
    {
        m_writer->markerType(value);
        return *this;
    }
    Self &markerSize(float value)
    {
        m_writer->markerSize(value);
        return *this;
    }
    Self &markerColor(int value)
    {
        m_writer->markerColor(value);
        return *this;
    }
    Self &textFontIndex(int value)
    {
        m_writer->textFontIndex(value);
        return *this;
    }
    Self &textPrecision(TextPrecision value)
    {
        m_writer->textPrecision(value);
        return *this;
    }
    Self &charExpansion(float value)
    {
        m_writer->charExpansion(value);
        return *this;
    }
    Self &charSpacing(float value)
    {
        m_writer->charSpacing(value);
        return *this;
    }
    Self &textColor(int index)
    {
        m_writer->textColor(index);
        return *this;
    }
    Self &charHeight(int value)
    {
        m_writer->charHeight(value);
        return *this;
    }
    Self &charOrientation(int upX, int upY, int baseX, int baseY)
    {
        m_writer->charOrientation(upX, upY, baseX, baseY);
        return *this;
    }
    Self &textPath(TextPath value)
    {
        m_writer->textPath(value);
        return *this;
    }
    Self &textAlignment(HorizAlign horiz, VertAlign vert, float contHoriz, float contVert)
    {
        m_writer->textAlignment(horiz, vert, contHoriz, contVert);
        return *this;
    }
    Self &interiorStyle(InteriorStyle value)
    {
        m_writer->interiorStyle(value);
        return *this;
    }
    Self &fillColor(int value)
    {
        m_writer->fillColor(value);
        return *this;
    }
    Self &hatchIndex(int value)
    {
        m_writer->hatchIndex(value);
        return *this;
    }
    Self &patternIndex(int value)
    {
        m_writer->patternIndex(value);
        return *this;
    }
    Self &colorTable(int startIndex, std::vector<Color> const &colors)
    {
        m_writer->colorTable(startIndex, colors);
        return *this;
    }
    
    PictureDescriptorBuilder beginPicture(const char *identifier);

private:
    std::unique_ptr<MetafileWriter> m_writer;
};

class PictureDescriptorBuilder
{
    using Self = PictureDescriptorBuilder;

public:
    PictureDescriptorBuilder(std::unique_ptr<MetafileWriter> &&writer)
        : m_writer(std::move(writer))
    {
    }

    Self &scaleMode(ScaleMode mode, float value)
    {
        return *this;
    }
    Self &colorMode(ColorMode mode)
    {
        return *this;
    }
    Self &lineWidthMode(SpecificationMode mode)
    {
        return *this;
    }
    Self &markerSizeMode(MarkerSizeMode mode)
    {
        return *this;
    }
    Self &vdcExtent(int llx, int lly, int urx, int ury)
    {
        return *this;
    }
    Self &backgroundColor(int red, int green, int blue)
    {
        return *this;
    }
    Self &beginPictureBody()
    {
        return *this;
    }

private:
    std::unique_ptr<MetafileWriter> m_writer;
};

inline PictureDescriptorBuilder PictureBodyBuilder::beginPicture(const char *identifier)
{
    m_writer->endPicture();
    m_writer->beginPicture(identifier);
    return PictureDescriptorBuilder{std::move(m_writer)};
}

inline PictureDescriptorBuilder NewPictureBuilder::beginPicture(const char *identifier)
{
    m_writer->beginPicture(identifier);
    return PictureDescriptorBuilder{std::move(m_writer)};
}

class MetafileDescriptorBuilder
{
    using Self = MetafileDescriptorBuilder;

public:
    MetafileDescriptorBuilder(std::unique_ptr<MetafileWriter> &&writer)
        : m_writer(std::move(writer))
    {
    }

    Self &version(int value)
    {
        m_writer->metafileVersion(value);
        return *this;
    }
    Self &description(char const *value)
    {
        m_writer->metafileDescription(value);
        return *this;
    }
    Self &vdcType(VdcType type)
    {
        m_writer->vdcType(type);
        return *this;
    }
    Self &intPrecisionClearText(int min, int max)
    {
        m_writer->intPrecisionClearText(min, max);
        return *this;
    }
    Self &intPrecisionBinary(int value)
    {
        m_writer->intPrecisionBinary(value);
        return *this;
    }
    Self &realPrecisionClearText(float minReal, float maxReal, int digits)
    {
        m_writer->realPrecisionClearText(minReal, maxReal, digits);
        return *this;
    }
    Self &realPrecisionBinary(RealPrecision prec, int expWidth, int mantWidth)
    {
        m_writer->realPrecisionBinary(prec, expWidth, mantWidth);
        return *this;
    }
    Self &indexPrecisionClearText(int min, int max)
    {
        m_writer->indexPrecisionClearText(min, max);
        return *this;
    }
    Self &colorPrecisionClearText(int max)
    {
        m_writer->colorPrecisionClearText(max);
        return *this;
    }
    Self &colorPrecisionBinary(int value)
    {
        m_writer->colorPrecisionBinary(value);
        return *this;
    }
    Self &colorIndexPrecisionClearText(int max)
    {
        m_writer->colorIndexPrecisionClearText(max);
        return *this;
    }
    Self &colorIndexPrecisionBinary(int value)
    {
        m_writer->colorIndexPrecisionBinary(value);
        return *this;
    }
    Self &maximumColorIndex(int max)
    {
        m_writer->maximumColorIndex(max);
        return *this;
    }
    Self &colorValueExtent(int redMin, int redMax, int greenMin, int greenMax, int blueMin, int blueMax)
    {
        m_writer->colorValueExtent(redMin, redMax, greenMin, greenMax, blueMin, blueMax);
        return *this;
    }
    Self &elementList()
    {
        m_writer->metafileElementList();
        return *this;
    }
    Self &defaultsReplacement()
    {
        m_writer->metafileDefaultsReplacement();
        return *this;
    }
    Self &fontList(std::vector<std::string> const &fonts)
    {
        m_writer->fontList(fonts);
        return *this;
    }
    Self &characterCodingAnnouncer(CharCodeAnnouncer value)
    {
        m_writer->characterCodingAnnouncer(value);
        return *this;
    }
    PictureDescriptorBuilder beginPicture(const char *identifier)
    {
        m_writer->beginPicture(identifier);
        return PictureDescriptorBuilder{std::move(m_writer)};
    }

private:
    std::unique_ptr<MetafileWriter> m_writer;
};

class Builder
{
public:
    Builder(std::unique_ptr<MetafileWriter> &&writer)
        : m_writer(std::move(writer))
    {
    }

    MetafileDescriptorBuilder begin(const char *identifier)
    {
        m_writer->beginMetafile(identifier);
        return MetafileDescriptorBuilder{std::move(m_writer)};
    }

private:
    std::unique_ptr<MetafileWriter> m_writer;
};

}
