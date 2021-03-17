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

/* Write a character to CGM clear text */
void ClearTextMetafileWriter::outChar(char chr)
{
    if (m_outputIndex >= cgmt_recl)
        flushBuffer();

    m_output[m_outputIndex++] = chr;
    m_output[m_outputIndex] = '\0';
}

/* Write string to CGM clear text */
void ClearTextMetafileWriter::cgmt_out_string(const char *string)
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

/* Start output command */
void ClearTextMetafileWriter::cgmt_start_cmd(int cl, int el)
{
    cgmt_out_string(cgmt_cptr[cl][el]);
}

/* Flush output command */
void ClearTextMetafileWriter::cgmt_flush_cmd()
{
    const char terminator = ';'; /* std. terminator character: ; or / */
    outChar(terminator);
    flushBuffer();
}

/* Write a CGM clear text string */
void ClearTextMetafileWriter::cgmt_string(const char *cptr, int slen)
{
    int i;

    outChar(' ');
    outChar(quoteChar);

    for (i = 0; i < slen; ++i)
    {
        if (cptr[i] == quoteChar)
        {
            outChar(quoteChar);
        }

        outChar(cptr[i]);
    }

    outChar(quoteChar);
}

/* Write a signed integer variable */
void ClearTextMetafileWriter::cgmt_int(int xin)
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
            outChar(' ');

        cgmt_out_string(cptr); /* all done */
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
        outChar(' ');

    cgmt_out_string(cptr);
}

/* Write a real variable */
void ClearTextMetafileWriter::cgmt_real(double xin)
{
    char buffer[maxStrLen];

    sprintf(buffer, " %.6f", xin);
    cgmt_out_string(buffer);
}

/* Write an integer point */
void ClearTextMetafileWriter::cgmt_ipoint(int x, int y)
{
    char buffer[maxStrLen];

    sprintf(buffer, " %d,%d", x, y);
    cgmt_out_string(buffer);
}

void ClearTextMetafileWriter::beginMetafile(const char *identifier)
{
    cgmt_start_cmd(0, (int) B_Mf);

    if (*identifier)
        cgmt_string(identifier, static_cast<int>(strlen(identifier)));
    else
        cgmt_string(nullptr, 0);

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::endMetafile()
{
    cgmt_start_cmd(0, (int) E_Mf);

    cgmt_flush_cmd();

    flushBuffer();
}

void ClearTextMetafileWriter::beginPicture(char const *identifier)
{
    cgmt_start_cmd(0, (int) B_Pic);

    if (*identifier)
        cgmt_string(identifier, strlen(identifier));
    else
        cgmt_string(NULL, 0);

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::beginPictureBody()
{
    cgmt_start_cmd(0, (int) B_Pic_Body);

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::endPicture()
{
    cgmt_start_cmd(0, (int) E_Pic);

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::metafileVersion(int value)
{
    cgmt_start_cmd(1, MfVersion);

    cgmt_int(value);

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::metafileDescription(char const *value)
{
    cgmt_start_cmd(1, MfDescrip);

    cgmt_string(value, strlen(value));

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::vdcType(VdcType type)
{
    cgmt_start_cmd(1, (int) cgm_class_1::vdcType);

    if (type == VdcType::Integer)
    {
        cgmt_out_string(" Integer");
    }
    else if (type == VdcType::Real)
    {
        cgmt_out_string(" Real");
    }

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::intPrecisionClearText(int min, int max)
{
    cgmt_start_cmd(1, (int) IntPrec);

    cgmt_int(min);
    cgmt_int(max);

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::intPrecisionBinary(int value)
{
    throw std::runtime_error("Unsupported method");
}

void ClearTextMetafileWriter::realPrecisionClearText(float minReal, float maxReal, int digits)
{
    cgmt_start_cmd(1, (int) RealPrec);

    cgmt_real(static_cast<double>(minReal));
    cgmt_real(static_cast<double>(maxReal));
    cgmt_int(digits);

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::realPrecisionBinary(RealPrecision prec, int expWidth, int mantWidth)
{
    throw std::runtime_error("Unsupported method");
}

void ClearTextMetafileWriter::indexPrecisionClearText(int min, int max)
{
    cgmt_start_cmd(1, (int) IndexPrec);

    cgmt_int(min);
    cgmt_int(max);

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::indexPrecisionBinary(int value)
{
    throw std::runtime_error("Unsupported method");
}

void ClearTextMetafileWriter::colorPrecisionClearText(int max)
{
    cgmt_start_cmd(1, (int) ColPrec);

    cgmt_int(max);

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::colorPrecisionBinary(int value)
{
    throw std::runtime_error("Unsupported method");
}

void ClearTextMetafileWriter::colorIndexPrecisionClearText(int max)
{
    cgmt_start_cmd(1, (int) CIndPrec);

    cgmt_int(max);

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::colorIndexPrecisionBinary(int value)
{
    throw std::runtime_error("Unsupported method");
}

void ClearTextMetafileWriter::maximumColorIndex(int max)
{
    cgmt_start_cmd(1, (int) MaxCInd);

    cgmt_int(max);

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::colorValueExtent(int redMin, int redMax, int greenMin, int greenMax, int blueMin, int blueMax)
{
    cgmt_start_cmd(1, (int) CVExtent);

    cgmt_int(redMin);
    cgmt_int(greenMin);
    cgmt_int(blueMin);
    cgmt_int(redMax);
    cgmt_int(greenMax);
    cgmt_int(blueMax);

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::metafileElementList()
{
    cgmt_start_cmd(1, (int) MfElList);

    outChar(' ');
    outChar(quoteChar);

    for (int i = 2; i < 2 * n_melements; i += 2)
    {
        cgmt_out_string(cgmt_cptr[element_list[i]][element_list[i + 1]]);

        if (i < 2 * (n_melements - 1))
            outChar(' ');
    }

    outChar(quoteChar);
    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::fontList(std::vector<std::string> const &fonts)
{
    char s[maxStrLen];

    cgmt_start_cmd(1, (int) FontList);

    const int numFonts = static_cast<int>(fonts.size());
    for (int i = 0; i < numFonts; i++)
    {
        cgmt_string(fonts[i].c_str(), static_cast<int>(fonts[i].size()));
        if (i < numFonts - 1)
        {
            outChar(',');
        }
    }

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::characterCodingAnnouncer(CharCodeAnnouncer value)
{
    static const char *const announcerNames[] = {
        "Basic7Bit",
        "Basic8Bit",
        "Extd7Bit",
        "Extd8Bit"
    };
    cgmt_start_cmd(1, (int) CharAnnounce);

    outChar(' ');
    cgmt_out_string(announcerNames[static_cast<int>(value)]);

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::scalingMode(ScalingMode mode, float value)
{
    cgmt_start_cmd(2, (int) ScalMode);

    cgmt_out_string(mode == ScalingMode::Metric ? " Metric" : " Abstract");
    cgmt_real(static_cast<double>(value));

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::colorSelectionMode(ColorMode mode)
{
    cgmt_start_cmd(2, (int) ColSelMode);

    cgmt_out_string(mode == ColorMode::Indexed ? " Indexed" : " Direct");

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::lineWidthSpecificationMode(SpecificationMode mode)
{
    cgmt_start_cmd(2, (int) LWidSpecMode);

    cgmt_out_string(mode == SpecificationMode::Absolute ? " Absolute" : " Scaled");

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::markerSizeSpecificationMode(SpecificationMode mode)
{
    cgmt_start_cmd(2, (int) MarkSizSpecMode);

    cgmt_out_string(mode == SpecificationMode::Absolute ? " Absolute" : " Scaled");

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::vdcExtent(int llx, int lly, int urx, int ury)
{
    cgmt_start_cmd(2, (int) cgm_class_2::vdcExtent);

    cgmt_ipoint(llx, lly);
    cgmt_ipoint(urx, ury);

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::backgroundColor(int red, int green, int blue)
{
    cgmt_start_cmd(2, (int) BackCol);

    cgmt_int(red);
    cgmt_int(green);
    cgmt_int(blue);

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::vdcIntegerPrecisionClearText(int min, int max)
{
    cgmt_start_cmd(3, (int) vdcIntPrec);

    cgmt_int(min);
    cgmt_int(max);

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::vdcIntegerPrecisionBinary(int value)
{
    throw std::runtime_error("Unsupported method");
}

void ClearTextMetafileWriter::clipRectangle(int llx, int lly, int urx, int ury)
{
    cgmt_start_cmd(3, (int) ClipRect);

    cgmt_ipoint(llx, lly);
    cgmt_ipoint(urx, ury);

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::clipIndicator(bool enabled)
{
    cgmt_start_cmd(3, (int) ClipIndic);

    cgmt_out_string(enabled ? " On" : " Off");

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::polyline(const std::vector<Point<int>> &points)
{
    cgmt_start_cmd(4, (int) PolyLine);

    for (const Point<int> &point : points)
    {
        cgmt_ipoint(point.x, point.y);
    }

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::polymarker(const std::vector<Point<int>> &points)
{
    cgmt_start_cmd(4, (int) PolyMarker);

    for (const Point<int> &i : points)
    {
        cgmt_ipoint(i.x, i.y);
    }

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::text(Point<int> point, TextFlag flag, const char *text)
{
    cgmt_start_cmd(4, (int) Text);

    cgmt_ipoint(point.x, point.y);

    cgmt_out_string(flag == TextFlag::Final ? " Final" : " NotFinal");

    cgmt_string(text, strlen(text));

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::polygon(const std::vector<Point<int>> &points)
{
    cgmt_start_cmd(4, (int) C_Polygon);

    for (const Point<int> &i : points)
    {
        cgmt_ipoint(i.x, i.y);
    }

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::cellArray(Point<int> c1, Point<int> c2, Point<int> c3, int colorPrecision, int nx, int ny, const int *colors)
{
    cgmt_start_cmd(4, (int) Cell_Array);

    cgmt_ipoint(c1.x, c1.y);
    cgmt_ipoint(c2.x, c2.y);
    cgmt_ipoint(c3.x, c3.y);
    cgmt_int(nx);
    cgmt_int(ny);
    cgmt_int(colorPrecision);

    for (int iy = 0; iy < ny; iy++)
    {
        flushBuffer();

        for (int ix = 0; ix < nx; ix++)
        {
            const int c = colors[nx * iy + ix];
            cgmt_int(c);

            if (ix < nx - 1)
                outChar(',');
        }
    }

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::lineType(int value)
{
    cgmt_start_cmd(5, (int) LType);

    cgmt_int((int) value);

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::lineWidth(float value)
{
    cgmt_start_cmd(5, (int) LWidth);

    cgmt_real(static_cast<double>(value));

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::lineColor(int value)
{
    cgmt_start_cmd(5, (int) LColour);

    cgmt_int(value);

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::markerType(int value)
{
    cgmt_start_cmd(5, (int) MType);

    cgmt_int(value);

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::markerSize(float value)
{
    cgmt_start_cmd(5, (int) MSize);

    cgmt_real(static_cast<double>(value));

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::markerColor(int value)
{
    cgmt_start_cmd(5, (int) MColour);

    cgmt_int(value);

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::textFontIndex(int value)
{
    cgmt_start_cmd(5, (int) TFIndex);

    cgmt_int(value);

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::textPrecision(TextPrecision value)
{
    cgmt_start_cmd(5, (int) TPrec);

    switch (static_cast<int>(value))
    {
    case string:
        cgmt_out_string(" String");
        break;

    case character:
        cgmt_out_string(" Character");
        break;

    case stroke:
        cgmt_out_string(" Stroke");
        break;
    }

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::charExpansion(float value)
{
    cgmt_start_cmd(5, (int) CExpFac);

    cgmt_real(static_cast<double>(value));

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::charSpacing(float value)
{
    cgmt_start_cmd(5, (int) CSpace);

    cgmt_real(static_cast<double>(value));

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::textColor(int index)
{
    cgmt_start_cmd(5, (int) TColour);

    cgmt_int(index);

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::charHeight(int value)
{
    cgmt_start_cmd(5, (int) CHeight);

    cgmt_int(value);

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::charOrientation(int upX, int upY, int baseX, int baseY)
{
    cgmt_start_cmd(5, (int) COrient);

    cgmt_int(upX);
    cgmt_int(upY);
    cgmt_int(baseX);
    cgmt_int(baseY);

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::textPath(TextPath value)
{
    cgmt_start_cmd(5, (int) TPath);

    switch (static_cast<int>(value))
    {
    case right:
        cgmt_out_string(" Right");
        break;

    case left:
        cgmt_out_string(" Left");
        break;

    case up:
        cgmt_out_string(" Up");
        break;

    case down:
        cgmt_out_string(" Down");
        break;
    }

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::textAlignment(HorizAlign horiz, VertAlign vert, float contHoriz, float contVert)
{
    cgmt_start_cmd(5, (int) TAlign);

    switch (static_cast<int>(horiz))
    {
    case normal_h:
        cgmt_out_string(" NormHoriz");
        break;

    case left_h:
        cgmt_out_string(" Left");
        break;

    case center_h:
        cgmt_out_string(" Ctr");
        break;

    case right_h:
        cgmt_out_string(" Right");
        break;

    case cont_h:
        cgmt_out_string(" ContHoriz");
        break;
    }

    switch (static_cast<int>(vert))
    {
    case normal_v:
        cgmt_out_string(" NormVert");
        break;

    case top_v:
        cgmt_out_string(" Top");
        break;

    case cap_v:
        cgmt_out_string(" Cap");
        break;

    case half_v:
        cgmt_out_string(" Half");
        break;

    case base_v:
        cgmt_out_string(" Base");
        break;

    case bottom_v:
        cgmt_out_string(" Bottom");
        break;

    case cont_v:
        cgmt_out_string(" ContVert");
        break;
    }

    cgmt_real(static_cast<double>(contHoriz));
    cgmt_real(static_cast<double>(contVert));

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::interiorStyle(InteriorStyle value)
{
    cgmt_start_cmd(5, (int) IntStyle);

    switch (static_cast<int>(value))
    {
    case hollow:
        cgmt_out_string(" Hollow");
        break;

    case solid_i:
        cgmt_out_string(" Solid");
        break;

    case pattern:
        cgmt_out_string(" Pat");
        break;

    case hatch:
        cgmt_out_string(" Hatch");
        break;

    case empty:
        cgmt_out_string(" Empty");
        break;
    }

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::fillColor(int value)
{
    cgmt_start_cmd(5, (int) FillColour);

    cgmt_int(value);

    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::hatchIndex(int value)
{
    cgmt_start_cmd(5, (int) HatchIndex);
    cgmt_int(value);
    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::patternIndex(int value)
{
    cgmt_start_cmd(5, (int) PatIndex);
    cgmt_int(value);
    cgmt_flush_cmd();
}

void ClearTextMetafileWriter::colorTable(int startIndex, std::vector<Color> const &colorTable)
{
    cgmt_start_cmd(5, (int) ColTab);
    cgmt_int(startIndex);

    for (int i = startIndex; i < (startIndex + static_cast<int>(colorTable.size())); ++i)
    {
        cgmt_int((int) (colorTable[(i - startIndex)].red * (max_colors - 1)));
        cgmt_int((int) (colorTable[(i - startIndex)].green * (max_colors - 1)));
        cgmt_int((int) (colorTable[(i - startIndex)].blue * (max_colors - 1)));
    }

    cgmt_flush_cmd();
}

}        // namespace cgm
