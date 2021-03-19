#include <iostream>
#include <memory>
#include <stdexcept>

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#if !defined(VMS) && !defined(_WIN32)
#include <unistd.h>
#endif

#include <cgm/cgm.h>

#include "clear_text.h"
#include "gkscore.h"
#include "impl.h"

static char digits[10] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };

static const char quoteChar = '"'; /* std. quote character: ' or " */
static const int maxStrLen = 128;  /* max length of std. strings */

namespace cgm
{

ClearTextMetafileWriter::ClearTextMetafileWriter(std::ostream &stream)
    : MetafileStreamWriter(stream)
{
}

ClearTextMetafileWriter::ClearTextMetafileWriter(int fd)
    : MetafileStreamWriter(fd)
{
}

void ClearTextMetafileWriter::flushBuffer()
{
    if (m_outputIndex != 0)
    {
        m_output[m_outputIndex++] = '\n';
    }
    MetafileStreamWriter::flushBuffer();
}

void ClearTextMetafileWriter::appendBufferByte(char chr)
{
    if (m_outputIndex >= cgmt_recl)
        flushBuffer();

    m_output[m_outputIndex++] = chr;
    m_output[m_outputIndex] = '\0';
}

void ClearTextMetafileWriter::writeString(const char *string)
{
    if ((int) (m_outputIndex + strlen(string)) >= cgmt_recl)
    {
        flushBuffer();
        strcpy(m_output, "   ");
        m_outputIndex = 3;
    }

    strcat(m_output, string);
    m_outputIndex = m_outputIndex + static_cast<int>(strlen(string));
}

void ClearTextMetafileWriter::startElement(int elemClass, int elemCode)
{
    writeString(cgmt_cptr[elemClass][elemCode]);
}

void ClearTextMetafileWriter::flushElement()
{
    const char terminator = ';'; /* std. terminator character: ; or / */
    appendBufferByte(terminator);
    flushBuffer();
}

void ClearTextMetafileWriter::writeQuotedString(const char *cptr, int slen)
{
    int i;

    appendBufferByte(' ');
    appendBufferByte(quoteChar);

    for (i = 0; i < slen; ++i)
    {
        if (cptr[i] == quoteChar)
        {
            appendBufferByte(quoteChar);
        }

        appendBufferByte(cptr[i]);
    }

    appendBufferByte(quoteChar);
}

void ClearTextMetafileWriter::writeSignedInt(int xin)
{
    static char buf[max_pwrs + 2];
    int is_neg;

    char* cptr = buf + max_pwrs + 1;
    *cptr = '\0';

    if (xin < 0)
    {
        is_neg = 1;
        xin = -xin;
    }
    else
        is_neg = 0;

    if (xin == 0)
    {
        *--cptr = digits[0];

        if ((int) (m_outputIndex + strlen(cptr)) < cgmt_recl)
            appendBufferByte(' ');

        writeString(cptr); /* all done */
        return;
    }

    while (xin)
    {
        const int j = xin % 10;
        *--cptr = digits[j];
        xin /= 10;
    }

    if (is_neg)
        *--cptr = '-';

    if ((int) (m_outputIndex + strlen(cptr)) < cgmt_recl)
        appendBufferByte(' ');

    writeString(cptr);
}

/* Write a real variable */
void ClearTextMetafileWriter::writeReal(double xin)
{
    char buffer[maxStrLen];

    sprintf(buffer, " %.6f", xin);
    writeString(buffer);
}

/* Write an integer point */
void ClearTextMetafileWriter::writeIntPoint(int x, int y)
{
    char buffer[maxStrLen];

    sprintf(buffer, " %d,%d", x, y);
    writeString(buffer);
}

void ClearTextMetafileWriter::beginMetafile(const char *identifier)
{
    startElement(0, (int) B_Mf);

    if (*identifier)
        writeQuotedString(identifier, static_cast<int>(strlen(identifier)));
    else
        writeQuotedString(nullptr, 0);

    flushElement();
}

void ClearTextMetafileWriter::endMetafile()
{
    startElement(0, (int) E_Mf);

    flushElement();

    flushBuffer();
}

void ClearTextMetafileWriter::beginPicture(char const *identifier)
{
    startElement(0, (int) B_Pic);

    if (*identifier)
        writeQuotedString(identifier, strlen(identifier));
    else
        writeQuotedString(NULL, 0);

    flushElement();
}

void ClearTextMetafileWriter::beginPictureBody()
{
    startElement(0, (int) B_Pic_Body);

    flushElement();
}

void ClearTextMetafileWriter::endPicture()
{
    startElement(0, (int) E_Pic);

    flushElement();
}

void ClearTextMetafileWriter::metafileVersion(int value)
{
    startElement(1, MfVersion);

    writeSignedInt(value);

    flushElement();
}

void ClearTextMetafileWriter::metafileDescription(char const *value)
{
    startElement(1, MfDescrip);

    writeQuotedString(value, strlen(value));

    flushElement();
}

void ClearTextMetafileWriter::vdcType(VdcType type)
{
    startElement(1, (int) cgm_class_1::vdcType);

    if (type == VdcType::Integer)
    {
        writeString(" Integer");
    }
    else if (type == VdcType::Real)
    {
        writeString(" Real");
    }

    flushElement();
}

void ClearTextMetafileWriter::intPrecisionClearText(int min, int max)
{
    startElement(1, (int) IntPrec);

    writeSignedInt(min);
    writeSignedInt(max);

    flushElement();
}

void ClearTextMetafileWriter::intPrecisionBinary(int value)
{
    throw std::runtime_error("Unsupported method");
}

void ClearTextMetafileWriter::realPrecisionClearText(float minReal, float maxReal, int digits)
{
    startElement(1, (int) RealPrec);

    writeReal(static_cast<double>(minReal));
    writeReal(static_cast<double>(maxReal));
    writeSignedInt(digits);

    flushElement();
}

void ClearTextMetafileWriter::realPrecisionBinary(RealPrecision prec, int expWidth, int mantWidth)
{
    throw std::runtime_error("Unsupported method");
}

void ClearTextMetafileWriter::indexPrecisionClearText(int min, int max)
{
    startElement(1, (int) IndexPrec);

    writeSignedInt(min);
    writeSignedInt(max);

    flushElement();
}

void ClearTextMetafileWriter::indexPrecisionBinary(int value)
{
    throw std::runtime_error("Unsupported method");
}

void ClearTextMetafileWriter::colorPrecisionClearText(int max)
{
    startElement(1, (int) ColPrec);

    writeSignedInt(max);

    flushElement();
}

void ClearTextMetafileWriter::colorPrecisionBinary(int value)
{
    throw std::runtime_error("Unsupported method");
}

void ClearTextMetafileWriter::colorIndexPrecisionClearText(int max)
{
    startElement(1, (int) CIndPrec);

    writeSignedInt(max);

    flushElement();
}

void ClearTextMetafileWriter::colorIndexPrecisionBinary(int value)
{
    throw std::runtime_error("Unsupported method");
}

void ClearTextMetafileWriter::maximumColorIndex(int max)
{
    startElement(1, (int) MaxCInd);

    writeSignedInt(max);

    flushElement();
}

void ClearTextMetafileWriter::colorValueExtent(int redMin, int redMax, int greenMin, int greenMax, int blueMin, int blueMax)
{
    startElement(1, (int) CVExtent);

    writeSignedInt(redMin);
    writeSignedInt(greenMin);
    writeSignedInt(blueMin);
    writeSignedInt(redMax);
    writeSignedInt(greenMax);
    writeSignedInt(blueMax);

    flushElement();
}

void ClearTextMetafileWriter::metafileElementList()
{
    startElement(1, (int) MfElList);

    appendBufferByte(' ');
    appendBufferByte(quoteChar);

    for (int i = 2; i < 2 * n_melements; i += 2)
    {
        writeString(cgmt_cptr[element_list[i]][element_list[i + 1]]);

        if (i < 2 * (n_melements - 1))
            appendBufferByte(' ');
    }

    appendBufferByte(quoteChar);
    flushElement();
}

void ClearTextMetafileWriter::fontList(std::vector<std::string> const &fonts)
{
    char s[maxStrLen];

    startElement(1, (int) FontList);

    const int numFonts = static_cast<int>(fonts.size());
    for (int i = 0; i < numFonts; i++)
    {
        writeQuotedString(fonts[i].c_str(), static_cast<int>(fonts[i].size()));
        if (i < numFonts - 1)
        {
            appendBufferByte(',');
        }
    }

    flushElement();
}

void ClearTextMetafileWriter::characterCodingAnnouncer(CharCodeAnnouncer value)
{
    static const char *const announcerNames[] = {
        "Basic7Bit",
        "Basic8Bit",
        "Extd7Bit",
        "Extd8Bit"
    };
    startElement(1, (int) CharAnnounce);

    appendBufferByte(' ');
    writeString(announcerNames[static_cast<int>(value)]);

    flushElement();
}

void ClearTextMetafileWriter::scalingMode(ScalingMode mode, float value)
{
    startElement(2, (int) ScalMode);

    writeString(mode == ScalingMode::Metric ? " Metric" : " Abstract");
    writeReal(static_cast<double>(value));

    flushElement();
}

void ClearTextMetafileWriter::colorSelectionMode(ColorMode mode)
{
    startElement(2, (int) ColSelMode);

    writeString(mode == ColorMode::Indexed ? " Indexed" : " Direct");

    flushElement();
}

void ClearTextMetafileWriter::lineWidthSpecificationMode(SpecificationMode mode)
{
    startElement(2, (int) LWidSpecMode);

    writeString(mode == SpecificationMode::Absolute ? " Absolute" : " Scaled");

    flushElement();
}

void ClearTextMetafileWriter::markerSizeSpecificationMode(SpecificationMode mode)
{
    startElement(2, (int) MarkSizSpecMode);

    writeString(mode == SpecificationMode::Absolute ? " Absolute" : " Scaled");

    flushElement();
}

void ClearTextMetafileWriter::vdcExtent(int llx, int lly, int urx, int ury)
{
    startElement(2, (int) cgm_class_2::vdcExtent);

    writeIntPoint(llx, lly);
    writeIntPoint(urx, ury);

    flushElement();
}

void ClearTextMetafileWriter::backgroundColor(int red, int green, int blue)
{
    startElement(2, (int) BackCol);

    writeSignedInt(red);
    writeSignedInt(green);
    writeSignedInt(blue);

    flushElement();
}

void ClearTextMetafileWriter::vdcIntegerPrecisionClearText(int min, int max)
{
    startElement(3, (int) vdcIntPrec);

    writeSignedInt(min);
    writeSignedInt(max);

    flushElement();
}

void ClearTextMetafileWriter::vdcIntegerPrecisionBinary(int value)
{
    throw std::runtime_error("Unsupported method");
}

void ClearTextMetafileWriter::clipRectangle(int llx, int lly, int urx, int ury)
{
    startElement(3, (int) ClipRect);

    writeIntPoint(llx, lly);
    writeIntPoint(urx, ury);

    flushElement();
}

void ClearTextMetafileWriter::clipIndicator(bool enabled)
{
    startElement(3, (int) ClipIndic);

    writeString(enabled ? " On" : " Off");

    flushElement();
}

void ClearTextMetafileWriter::polyline(const std::vector<Point<int>> &points)
{
    startElement(4, (int) PolyLine);

    for (const Point<int> &point : points)
    {
        writeIntPoint(point.x, point.y);
    }

    flushElement();
}

void ClearTextMetafileWriter::polymarker(const std::vector<Point<int>> &points)
{
    startElement(4, (int) PolyMarker);

    for (const Point<int> &i : points)
    {
        writeIntPoint(i.x, i.y);
    }

    flushElement();
}

void ClearTextMetafileWriter::text(Point<int> point, TextFlag flag, const char *text)
{
    startElement(4, (int) Text);

    writeIntPoint(point.x, point.y);

    writeString(flag == TextFlag::Final ? " Final" : " NotFinal");

    writeQuotedString(text, strlen(text));

    flushElement();
}

void ClearTextMetafileWriter::polygon(const std::vector<Point<int>> &points)
{
    startElement(4, (int) C_Polygon);

    for (const Point<int> &i : points)
    {
        writeIntPoint(i.x, i.y);
    }

    flushElement();
}

void ClearTextMetafileWriter::cellArray(Point<int> c1, Point<int> c2, Point<int> c3, int colorPrecision, int nx, int ny, const int *colors)
{
    startElement(4, (int) Cell_Array);

    writeIntPoint(c1.x, c1.y);
    writeIntPoint(c2.x, c2.y);
    writeIntPoint(c3.x, c3.y);
    writeSignedInt(nx);
    writeSignedInt(ny);
    writeSignedInt(colorPrecision);

    for (int iy = 0; iy < ny; iy++)
    {
        flushBuffer();

        for (int ix = 0; ix < nx; ix++)
        {
            const int c = colors[nx * iy + ix];
            writeSignedInt(c);

            if (ix < nx - 1)
                appendBufferByte(',');
        }
    }

    flushElement();
}

void ClearTextMetafileWriter::lineType(int value)
{
    startElement(5, (int) LType);

    writeSignedInt((int) value);

    flushElement();
}

void ClearTextMetafileWriter::lineWidth(float value)
{
    startElement(5, (int) LWidth);

    writeReal(static_cast<double>(value));

    flushElement();
}

void ClearTextMetafileWriter::lineColor(int value)
{
    startElement(5, (int) LColour);

    writeSignedInt(value);

    flushElement();
}

void ClearTextMetafileWriter::markerType(int value)
{
    startElement(5, (int) MType);

    writeSignedInt(value);

    flushElement();
}

void ClearTextMetafileWriter::markerSize(float value)
{
    startElement(5, (int) MSize);

    writeReal(static_cast<double>(value));

    flushElement();
}

void ClearTextMetafileWriter::markerColor(int value)
{
    startElement(5, (int) MColour);

    writeSignedInt(value);

    flushElement();
}

void ClearTextMetafileWriter::textFontIndex(int value)
{
    startElement(5, (int) TFIndex);

    writeSignedInt(value);

    flushElement();
}

void ClearTextMetafileWriter::textPrecision(TextPrecision value)
{
    startElement(5, (int) TPrec);

    switch (static_cast<int>(value))
    {
    case string:
        writeString(" String");
        break;

    case character:
        writeString(" Character");
        break;

    case stroke:
        writeString(" Stroke");
        break;
    }

    flushElement();
}

void ClearTextMetafileWriter::charExpansion(float value)
{
    startElement(5, (int) CExpFac);

    writeReal(static_cast<double>(value));

    flushElement();
}

void ClearTextMetafileWriter::charSpacing(float value)
{
    startElement(5, (int) CSpace);

    writeReal(static_cast<double>(value));

    flushElement();
}

void ClearTextMetafileWriter::textColor(int index)
{
    startElement(5, (int) TColour);

    writeSignedInt(index);

    flushElement();
}

void ClearTextMetafileWriter::charHeight(int value)
{
    startElement(5, (int) CHeight);

    writeSignedInt(value);

    flushElement();
}

void ClearTextMetafileWriter::charOrientation(int upX, int upY, int baseX, int baseY)
{
    startElement(5, (int) COrient);

    writeSignedInt(upX);
    writeSignedInt(upY);
    writeSignedInt(baseX);
    writeSignedInt(baseY);

    flushElement();
}

void ClearTextMetafileWriter::textPath(TextPath value)
{
    startElement(5, (int) TPath);

    switch (static_cast<int>(value))
    {
    case right:
        writeString(" Right");
        break;

    case left:
        writeString(" Left");
        break;

    case up:
        writeString(" Up");
        break;

    case down:
        writeString(" Down");
        break;
    }

    flushElement();
}

void ClearTextMetafileWriter::textAlignment(HorizAlign horiz, VertAlign vert, float contHoriz, float contVert)
{
    startElement(5, (int) TAlign);

    switch (static_cast<int>(horiz))
    {
    case normal_h:
        writeString(" NormHoriz");
        break;

    case left_h:
        writeString(" Left");
        break;

    case center_h:
        writeString(" Ctr");
        break;

    case right_h:
        writeString(" Right");
        break;

    case cont_h:
        writeString(" ContHoriz");
        break;
    }

    switch (static_cast<int>(vert))
    {
    case normal_v:
        writeString(" NormVert");
        break;

    case top_v:
        writeString(" Top");
        break;

    case cap_v:
        writeString(" Cap");
        break;

    case half_v:
        writeString(" Half");
        break;

    case base_v:
        writeString(" Base");
        break;

    case bottom_v:
        writeString(" Bottom");
        break;

    case cont_v:
        writeString(" ContVert");
        break;
    }

    writeReal(static_cast<double>(contHoriz));
    writeReal(static_cast<double>(contVert));

    flushElement();
}

void ClearTextMetafileWriter::interiorStyle(InteriorStyle value)
{
    startElement(5, (int) IntStyle);

    switch (static_cast<int>(value))
    {
    case hollow:
        writeString(" Hollow");
        break;

    case solid_i:
        writeString(" Solid");
        break;

    case pattern:
        writeString(" Pat");
        break;

    case hatch:
        writeString(" Hatch");
        break;

    case empty:
        writeString(" Empty");
        break;
    }

    flushElement();
}

void ClearTextMetafileWriter::fillColor(int value)
{
    startElement(5, (int) FillColour);

    writeSignedInt(value);

    flushElement();
}

void ClearTextMetafileWriter::hatchIndex(int value)
{
    startElement(5, (int) HatchIndex);
    writeSignedInt(value);
    flushElement();
}

void ClearTextMetafileWriter::patternIndex(int value)
{
    startElement(5, (int) PatIndex);
    writeSignedInt(value);
    flushElement();
}

void ClearTextMetafileWriter::colorTable(int startIndex, std::vector<Color> const &colorTable)
{
    startElement(5, (int) ColTab);
    writeSignedInt(startIndex);

    for (int i = startIndex; i < (startIndex + static_cast<int>(colorTable.size())); ++i)
    {
        writeSignedInt((int) (colorTable[(i - startIndex)].red * (max_colors - 1)));
        writeSignedInt((int) (colorTable[(i - startIndex)].green * (max_colors - 1)));
        writeSignedInt((int) (colorTable[(i - startIndex)].blue * (max_colors - 1)));
    }

    flushElement();
}

}        // namespace cgm
