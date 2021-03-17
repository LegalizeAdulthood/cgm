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

#include "binary.h"
#include "gkscore.h"
#include "impl.h"

#define odd(number) ((number) &01)

namespace cgm
{

BinaryMetafileWriter::BinaryMetafileWriter(std::ostream &stream)
    : MetafileStreamWriter(stream)
{
}

BinaryMetafileWriter::BinaryMetafileWriter(int fd)
    : MetafileStreamWriter(fd)
{
}

void BinaryMetafileWriter::outByte(char chr)
{
    if (m_outputIndex >= max_buffer)
        flushBuffer();

    m_output[m_outputIndex++] = chr;
}

void BinaryMetafileWriter::startElement(int elemClass, int elemCode)
{
    m_cmdHdr = m_cmdBuffer + m_buffIndex;
    m_cmdData = m_cmdHdr + hdr_long;
    m_buffIndex += hdr_long;

    m_cmdHdr[0] = static_cast<char>(elemClass << 4 | elemCode >> 3);
    m_cmdHdr[1] = static_cast<char>(elemCode << 5);
    m_cmdIndex = 0;
    m_partition = 1;
}

void BinaryMetafileWriter::flushElement(Flush flag)
{
    if ((flag == Flush::Final) && (m_partition == 1) && (m_cmdIndex <= max_short))
    {
        m_cmdHdr[1] |= m_cmdIndex;

        /* flush out the header */
        for (int i = 0; i < hdr_short; ++i)
        {
            outByte(m_cmdHdr[i]);
        }
    }
    else
    {
        /* need a long form */
        if (m_partition == 1)
        {
            /* first one */
            m_cmdHdr[1] |= 31;

            for (int i = 0; i < hdr_short; ++i)
            {
                outByte(m_cmdHdr[i]);
            }
        }

        m_cmdHdr[2] = m_cmdIndex >> 8;
        m_cmdHdr[3] = m_cmdIndex & 255;

        if (flag == Flush::Intermediate)
        {
            m_cmdHdr[2] |= 1 << 7; /* more come */
        }

        /* flush out the header */
        for (int i = hdr_short; i < hdr_long; ++i)
        {
            outByte(m_cmdHdr[i]);
        }
    }

    /* now flush out the data */
    for (int i = 0; i < m_cmdIndex; ++i)
    {
        outByte(m_cmdData[i]);
    }

    if (m_cmdIndex % 2)
    {
        outByte('\0');
    }

    m_cmdIndex = 0;
    m_buffIndex = 0;
    ++m_partition;
}

/* Write one byte */
void BinaryMetafileWriter::cgmb_out_bc(int c)
{
    if (m_cmdIndex >= max_long)
    {
        flushElement(Flush::Final);
    }

    m_cmdData[m_cmdIndex++] = c;
}

/* Write multiple bytes */
void BinaryMetafileWriter::cgmb_out_bs(const char *cptr, int n)
{
    int to_do, space_left, i;

    to_do = n;
    space_left = max_long - m_cmdIndex;

    while (to_do > space_left)
    {
        for (i = 0; i < space_left; ++i)
        {
            m_cmdData[m_cmdIndex++] = *cptr++;
        }

        flushElement(Flush::Final);
        to_do -= space_left;
        space_left = max_long;
    }

    for (i = 0; i < to_do; ++i)
    {
        m_cmdData[m_cmdIndex++] = *cptr++;
    }
}

/* Write a CGM binary string */
void BinaryMetafileWriter::cgmb_string(const char *cptr, int slen)
{
    int to_do;
    unsigned char byte1, byte2;

    /* put out null strings, however */

    if (slen == 0)
    {
        cgmb_out_bc(0);
        return;
    }

    /* now non-trivial stuff */

    if (slen < 255)
    {
        /* simple case */

        cgmb_out_bc(slen);
        cgmb_out_bs(cptr, slen);
    }
    else
    {
        cgmb_out_bc(255);
        to_do = slen;

        while (to_do > 0)
        {
            if (to_do < max_long)
            {
                /* last one */

                byte1 = to_do >> 8;
                byte2 = to_do & 255;

                cgmb_out_bc(byte1);
                cgmb_out_bc(byte2);
                cgmb_out_bs(cptr, to_do);

                to_do = 0;
            }
            else
            {
                byte1 = (max_long >> 8) | (1 << 7);
                byte2 = max_long & 255;

                cgmb_out_bc(byte1);
                cgmb_out_bc(byte2);
                cgmb_out_bs(cptr, max_long);

                to_do -= max_long;
            }
        }
    }
}

/* Write a signed integer variable */
void BinaryMetafileWriter::cgmb_gint(int xin, int precision)
{
    char buffer[4]{};

    int no_out = precision / byte_size;

    int xshifted = xin;
    for (int i = no_out - 1; i >= 0; --i)
    {
        buffer[i] = xshifted & byte_mask;
        xshifted >>= byte_size;
    }

    if ((xin < 0) && (buffer[0] > '\0')) /* maybe after truncation */
    {
        buffer[0] |= 1 << 7; /* assuming two's complement */
    }

    cgmb_out_bs(buffer, no_out);
}

/* Write an unsigned integer variable */
void BinaryMetafileWriter::cgmb_uint(unsigned int xin, int precision)
{
    int i, no_out;
    unsigned char buffer[4];

    no_out = precision / byte_size;

    for (i = no_out - 1; i >= 0; --i)
    {
        buffer[i] = xin & byte_mask;
        xin >>= byte_size;
    }

    cgmb_out_bs((char *) buffer, no_out);
}

/* Write fixed point variable */
void BinaryMetafileWriter::cgmb_fixed(double xin)
{
    int exp_part, fract_part;
    double fract_real;

    exp_part = (int) xin;
    if (exp_part > xin)
    {
        exp_part -= 1; /* take it below xin */
    }

    fract_real = xin - exp_part;
    fract_part = (int) (fract_real * (01 << real_prec_fract));

    cgmb_gint(exp_part, real_prec_exp);
    cgmb_uint(fract_part, real_prec_fract);
}

/* Write IEEE floating point variable */
void BinaryMetafileWriter::cgmb_float(double xin)
{
    unsigned char arry[8];
    int sign_bit, i;
    unsigned int exponent;
    unsigned long fract;
    double dfract;

    if (xin < 0.0)
    {
        sign_bit = 1;
        xin = -xin;
    }
    else
    {
        sign_bit = 0;
    }

    /* first calculate the exponent and fraction */

    if (xin == 0.0)
    {
        exponent = 0;
        fract = 0;
    }
    else
    {
        switch (real_prec_exp + real_prec_fract)
        {
            /* first 32 bit precision */

        case 32:
        {
            if (xin < 1.0)
            {
                for (i = 0; (xin < 1.0) && (i < 128); ++i)
                {
                    xin *= 2.0;
                }

                exponent = 127 - i;
            }
            else
            {
                if (xin >= 2.0)
                {
                    for (i = 0; (xin >= 2.0) && (i < 127); ++i)
                    {
                        xin /= 2.0;
                    }
                    exponent = 127 + i;
                }
                else
                {
                    exponent = 127;
                }
            }
            dfract = xin - 1.0;

            for (i = 0; i < 23; ++i)
            {
                dfract *= 2.0;
            }

            fract = (unsigned long) dfract;
            break;
        }

        /* now 64 bit precision */

        case 64:
        {
            break;
        }
        }
    }

    switch (real_prec_exp + real_prec_fract)
    {
        /* first 32 bit precision */

    case 32:
    {
        arry[0] = ((sign_bit & 1) << 7) | ((exponent >> 1) & 127);
        arry[1] = ((exponent & 1) << 7) | ((fract >> 16) & 127);
        arry[2] = (fract >> 8) & 255;
        arry[3] = fract & 255;
        cgmb_out_bs((char *) arry, 4);
        break;
    }

    /* now 64 bit precision */

    case 64:
    {
        break;
    }
    }
}

/* Write direct colour value */
void BinaryMetafileWriter::cgmb_dcint(int xin)
{
    cgmb_uint(xin, cprec);
}

/* Write a signed int at VDC integer precision */
void BinaryMetafileWriter::cgmb_vint(int xin)
{
    cgmb_gint(xin, 16);
}

/* Write a standard CGM signed int */
void BinaryMetafileWriter::cgmb_sint(int xin)
{
    cgmb_gint(xin, 16);
}

/* Write a signed int at index precision */
void BinaryMetafileWriter::cgmb_xint(int xin)
{
    cgmb_gint(xin, 16);
}

/* Write an unsigned integer at colour index precision */
void BinaryMetafileWriter::cgmb_cxint(int xin)
{
    cgmb_uint((unsigned) xin, cxprec);
}

/* Write an integer at fixed (16 bit) precision */
void BinaryMetafileWriter::cgmb_eint(int xin)
{
    char byte1;
    unsigned char byte2;

    byte1 = xin / 256;
    byte2 = xin & 255;
    cgmb_out_bc(byte1);
    cgmb_out_bc(byte2);
}

void BinaryMetafileWriter::beginMetafile(const char *identifier)
{
    startElement(0, (int) B_Mf);

    if (*identifier)
    {
        cgmb_string(identifier, strlen(identifier));
    }
    else
    {
        cgmb_string(NULL, 0);
    }

    flushElement(Flush::Final);

    flushBuffer();
}

void BinaryMetafileWriter::endMetafile()
{
    /* put out the end metafile command */

    startElement(0, (int) E_Mf);
    flushElement(Flush::Final);

    /* flush out the buffer */

    flushBuffer();
}

void BinaryMetafileWriter::beginPicture(char const *identifier)
{
    startElement(0, (int) B_Pic);

    if (*identifier)
    {
        cgmb_string(identifier, strlen(identifier));
    }
    else
    {
        cgmb_string(NULL, 0);
    }

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::beginPictureBody()
{
    startElement(0, (int) B_Pic_Body);

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::endPicture()
{
    startElement(0, (int) E_Pic);

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::metafileVersion(int value)
{
    startElement(1, (int) MfVersion);

    cgmb_sint(value);

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::metafileDescription(char const *value)
{
    startElement(1, MfDescrip);

    cgmb_string(value, static_cast<int>(strlen(value)));

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::vdcType(VdcType type)
{
    startElement(1, (int) cgm_class_1::vdcType);

    cgmb_eint((int) type);

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::intPrecisionClearText(int min, int max)
{
    throw std::runtime_error("Unsupported method");
}

void BinaryMetafileWriter::intPrecisionBinary(int value)
{
    startElement(1, (int) IntPrec);

    cgmb_sint(value);

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::realPrecisionClearText(float minReal, float maxReal, int digits)
{
    throw std::runtime_error("Unsupported method");
}

void BinaryMetafileWriter::realPrecisionBinary(RealPrecision prec, int expWidth, int mantWidth)
{
    startElement(1, (int) RealPrec);

    cgmb_sint(static_cast<int>(prec));
    cgmb_sint(expWidth);
    cgmb_sint(mantWidth);

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::indexPrecisionClearText(int min, int max)
{
    throw std::runtime_error("Unsupported method");
}

void BinaryMetafileWriter::indexPrecisionBinary(int value)
{
    startElement(1, (int) IndexPrec);

    cgmb_sint(value);

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::colorPrecisionClearText(int max)
{
    throw std::runtime_error("Unsupported method");
}

void BinaryMetafileWriter::colorPrecisionBinary(int value)
{
    startElement(1, (int) ColPrec);

    cgmb_sint(value);

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::colorIndexPrecisionClearText(int max)
{
    throw std::runtime_error("Unsupported method");
}

void BinaryMetafileWriter::colorIndexPrecisionBinary(int value)
{
    startElement(1, (int) CIndPrec);

    cgmb_sint(value);

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::maximumColorIndex(int max)
{
    startElement(1, (int) MaxCInd);

    cgmb_cxint(max);

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::colorValueExtent(int redMin, int redMax, int greenMin, int greenMax, int blueMin, int blueMax)
{
    startElement(1, (int) CVExtent);

    cgmb_dcint(redMin);
    cgmb_dcint(greenMin);
    cgmb_dcint(blueMin);
    cgmb_dcint(redMax);
    cgmb_dcint(greenMax);
    cgmb_dcint(blueMax);

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::metafileElementList()
{
    startElement(1, (int) MfElList);
    cgmb_sint(n_melements);

    for (int i = 2; i < 2 * n_melements; ++i)
    {
        cgmb_xint(element_list[i]);
    }

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::fontList(std::vector<std::string> const &fonts)
{
    startElement(1, (int) FontList);

    for (const std::string &s : fonts)
    {
        cgmb_string(s.c_str(), static_cast<int>(s.length()));
    }

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::characterCodingAnnouncer(CharCodeAnnouncer value)
{
    startElement(1, (int) CharAnnounce);

    cgmb_eint(static_cast<int>(value));

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::scalingMode(ScalingMode mode, float value)
{
    startElement(2, (int) ScalMode);

    cgmb_eint(static_cast<int>(mode));
    cgmb_float(static_cast<double>(value));

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::colorSelectionMode(ColorMode mode)
{
    startElement(2, (int) ColSelMode);

    cgmb_eint(static_cast<int>(mode));

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::lineWidthSpecificationMode(SpecificationMode mode)
{
    startElement(2, (int) LWidSpecMode);

    cgmb_eint(static_cast<int>(mode));

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::markerSizeSpecificationMode(SpecificationMode mode)
{
    startElement(2, (int) MarkSizSpecMode);

    cgmb_eint(static_cast<int>(mode));

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::vdcExtent(int llx, int lly, int urx, int ury)
{
    startElement(2, (int) cgm_class_2::vdcExtent);

    cgmb_vint(llx);
    cgmb_vint(lly);
    cgmb_vint(urx);
    cgmb_vint(ury);

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::backgroundColor(int red, int green, int blue)
{
    startElement(2, (int) BackCol);

    cgmb_dcint(red);
    cgmb_dcint(green);
    cgmb_dcint(blue);

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::vdcIntegerPrecisionClearText(int min, int max)
{
    throw std::runtime_error("Unsupported");
}

void BinaryMetafileWriter::vdcIntegerPrecisionBinary(int value)
{
    startElement(3, (int) vdcIntPrec);

    cgmb_sint(value);

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::clipRectangle(int llx, int lly, int urx, int ury)
{
    startElement(3, (int) ClipRect);

    cgmb_vint(llx);
    cgmb_vint(lly);
    cgmb_vint(urx);
    cgmb_vint(ury);

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::clipIndicator(bool enabled)
{
    startElement(3, (int) ClipIndic);

    cgmb_eint(enabled ? 1 : 0);

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::polyline(const std::vector<Point<int>> &points)
{
    startElement(4, (int) PolyLine);

    for (const Point<int> &point : points)
    {
        cgmb_vint(point.x);
        cgmb_vint(point.y);
    }

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::polymarker(const std::vector<Point<int>> &points)
{
    startElement(4, (int) PolyMarker);

    for (const Point<int> &point : points)
    {
        cgmb_vint(point.x);
        cgmb_vint(point.y);
    }

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::text(Point<int> point, TextFlag flag, const char *text)
{
    startElement(4, (int) Text);

    cgmb_vint(point.x);
    cgmb_vint(point.y);

    cgmb_eint(static_cast<int>(flag));
    cgmb_string(text, strlen(text));

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::polygon(const std::vector<Point<int>> &points)
{
    startElement(4, (int) C_Polygon);

    for (const Point<int> &point : points)
    {
        cgmb_vint(point.x);
        cgmb_vint(point.y);
    }

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::cellArray(Point<int> c1, Point<int> c2, Point<int> c3, int colorPrecision, int nx, int ny, const int *colors)
{
    startElement(4, (int) Cell_Array);

    cgmb_vint(c1.x);
    cgmb_vint(c1.y);
    cgmb_vint(c2.x);
    cgmb_vint(c2.y);
    cgmb_vint(c3.x);
    cgmb_vint(c3.y);

    cgmb_sint(nx);
    cgmb_sint(ny);
    cgmb_sint(colorPrecision);
    cgmb_eint(1);

    for (int iy = 0; iy < ny; iy++)
    {
        for (int ix = 0; ix < nx; ix++)
        {
            const int c = colors[nx * iy + ix];
            cgmb_out_bc(c);
        }

        if (odd(nx))
            cgmb_out_bc(0);
    }

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::lineType(int value)
{
    startElement(5, (int) LType);

    cgmb_xint(value);

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::lineWidth(float value)
{
    startElement(5, (int) LWidth);

    cgmb_fixed(static_cast<double>(value));

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::lineColor(int value)
{
    startElement(5, (int) LColour);

    cgmb_cxint(value);

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::markerType(int value)
{
    startElement(5, (int) MType);

    cgmb_xint(value);

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::markerSize(float value)
{
    startElement(5, (int) MSize);

    cgmb_fixed(static_cast<double>(value));

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::markerColor(int value)
{
    startElement(5, (int) MColour);

    cgmb_cxint(value);

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::textFontIndex(int value)
{
    startElement(5, (int) TFIndex);

    cgmb_xint(value);

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::textPrecision(TextPrecision value)
{
    startElement(5, (int) TPrec);

    cgmb_eint(static_cast<int>(value));

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::charExpansion(float value)
{
    startElement(5, (int) CExpFac);

    cgmb_fixed(static_cast<double>(value));

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::charSpacing(float value)
{
    startElement(5, (int) CSpace);

    cgmb_fixed(static_cast<double>(value));

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::textColor(int index)
{
    startElement(5, (int) TColour);

    cgmb_cxint(index);

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::charHeight(int value)
{
    startElement(5, (int) CHeight);

    cgmb_vint(value);

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::charOrientation(int upX, int upY, int baseX, int baseY)
{
    startElement(5, (int) COrient);

    cgmb_vint(upX);
    cgmb_vint(upY);
    cgmb_vint(baseX);
    cgmb_vint(baseY);

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::textPath(TextPath value)
{
    startElement(5, (int) TPath);

    cgmb_eint(static_cast<int>(value));

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::textAlignment(HorizAlign horiz, VertAlign vert, float contHoriz, float contVert)
{
    startElement(5, (int) TAlign);

    cgmb_eint(static_cast<int>(horiz));
    cgmb_eint(static_cast<int>(vert));
    cgmb_fixed(static_cast<double>(contHoriz));
    cgmb_fixed(static_cast<double>(contVert));

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::interiorStyle(InteriorStyle value)
{
    startElement(5, (int) IntStyle);

    cgmb_eint(static_cast<int>(value));

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::fillColor(int value)
{
    startElement(5, (int) FillColour);

    cgmb_cxint(value);

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::hatchIndex(int value)
{
    startElement(5, (int) HatchIndex);

    cgmb_xint(value);

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::patternIndex(int value)
{
    startElement(5, (int) PatIndex);

    cgmb_xint(value);

    flushElement(Flush::Final);
    flushBuffer();
}

void BinaryMetafileWriter::colorTable(int startIndex, std::vector<Color> const &colors)
{
    const int numColors = static_cast<int>(colors.size());

    startElement(5, (int) ColTab);
    cgmb_cxint(startIndex);

    for (int i = startIndex; i < (startIndex + numColors); ++i)
    {
        cgmb_dcint((int) (colors[(i - startIndex)].red * (max_colors - 1)));
        cgmb_dcint((int) (colors[(i - startIndex)].green * (max_colors - 1)));
        cgmb_dcint((int) (colors[(i - startIndex)].blue * (max_colors - 1)));
    }

    flushElement(Flush::Final);
    flushBuffer();
}

}        // namespace cgm
