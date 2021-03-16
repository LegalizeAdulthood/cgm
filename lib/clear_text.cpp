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

/* Flush output buffer */
static void cgmt_fb(cgm_context *ctx)
{
    if (ctx->buffer_ind != 0)
    {
        ctx->buffer[ctx->buffer_ind++] = '\n';
        ctx->buffer[ctx->buffer_ind] = '\0';
        ctx->flush_buffer(ctx, ctx->flush_buffer_context);

        ctx->buffer_ind = 0;
        ctx->buffer[0] = '\0';
    }
}

/* Write a character to CGM clear text */
static void cgmt_outc(cgm_context *ctx, char chr)
{
    if (ctx->buffer_ind >= cgmt_recl)
        cgmt_fb(ctx);

    ctx->buffer[ctx->buffer_ind++] = chr;
    ctx->buffer[ctx->buffer_ind] = '\0';
}

/* Write string to CGM clear text */
static void cgmt_out_string(cgm_context *ctx, const char *string)
{
    if ((int) (ctx->buffer_ind + strlen(string)) >= cgmt_recl)
    {
        cgmt_fb(ctx);
        strcpy(ctx->buffer, "   ");
        ctx->buffer_ind = 3;
    }

    strcat(ctx->buffer, string);
    ctx->buffer_ind = ctx->buffer_ind + static_cast<int>(strlen(string));
}

/* Start output command */
static void cgmt_start_cmd(cgm_context *p, int cl, int el)
{
    cgmt_out_string(p, cgmt_cptr[cl][el]);
}

/* Flush output command */
static void cgmt_flush_cmd(cgm_context *ctx, int this_flush)
{
    cgmt_outc(ctx, term_char);
    cgmt_fb(ctx);
}

/* Write a CGM clear text string */
static void cgmt_string(cgm_context *ctx, const char *cptr, int slen)
{
    int i;

    cgmt_outc(ctx, ' ');
    cgmt_outc(ctx, quote_char);

    for (i = 0; i < slen; ++i)
    {
        if (cptr[i] == quote_char)
        {
            cgmt_outc(ctx, quote_char);
        }

        cgmt_outc(ctx, cptr[i]);
    }

    cgmt_outc(ctx, quote_char);
}

/* Write a signed integer variable */
static void cgmt_int(cgm_context *ctx, int xin)
{
    static char buf[max_pwrs + 2];
    register char *cptr;
    register int is_neg, j;

    cptr = buf + max_pwrs + 1;
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

        if ((int) (ctx->buffer_ind + strlen(cptr)) < cgmt_recl)
            cgmt_outc(ctx, ' ');

        cgmt_out_string(ctx, cptr); /* all done */
        return;
    }

    while (xin)
    {
        j = xin % 10;
        *--cptr = digits[j];
        xin /= 10;
    }

    if (is_neg)
        *--cptr = '-';

    if ((int) (ctx->buffer_ind + strlen(cptr)) < cgmt_recl)
        cgmt_outc(ctx, ' ');

    cgmt_out_string(ctx, cptr);
}

namespace cgm
{

ClearTextMetafileWriter::ClearTextMetafileWriter(std::ostream &stream)
    : MetafileStreamWriter(stream)
{
    m_context.encode = cgm_clear_text;
}

ClearTextMetafileWriter::ClearTextMetafileWriter(int fd)
    : MetafileStreamWriter(fd)
{
    m_context.encode = cgm_clear_text;
}

/* Write a real variable */
void ClearTextMetafileWriter::cgmt_real(cgm_context *ctx, double xin)
{
    char buffer[max_str];

    sprintf(buffer, " %.6f", xin);
    cgmt_out_string(ctx, buffer);
}

/* Write an integer point */
void ClearTextMetafileWriter::cgmt_ipoint(cgm_context *ctx, int x, int y)
{
    char buffer[max_str];

    sprintf(buffer, " %d,%d", x, y);
    cgmt_out_string(ctx, buffer);
}

void ClearTextMetafileWriter::beginMetafile(const char *identifier)
{
    cgm_context *ctx = &m_context;
    cgmt_start_cmd(ctx, 0, (int) B_Mf);

    if (*identifier)
        cgmt_string(ctx, identifier, static_cast<int>(strlen(identifier)));
    else
        cgmt_string(ctx, nullptr, 0);

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::endMetafile()
{
    cgm_context *ctx = &m_context;
    cgmt_start_cmd(ctx, 0, (int) E_Mf);

    cgmt_flush_cmd(ctx, final_flush);

    cgmt_fb(ctx);
}

void ClearTextMetafileWriter::beginPicture(char const *identifier)
{
    cgm_context *ctx = &m_context;
    cgmt_start_cmd(ctx, 0, (int) B_Pic);

    if (*identifier)
        cgmt_string(ctx, identifier, strlen(identifier));
    else
        cgmt_string(ctx, NULL, 0);

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::beginPictureBody()
{
    cgm_context *ctx = &m_context;
    cgmt_start_cmd(ctx, 0, (int) B_Pic_Body);

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::endPicture()
{
    cgm_context *ctx = &m_context;
    cgmt_start_cmd(ctx, 0, (int) E_Pic);

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::metafileVersion(int value)
{
    cgm_context *ctx = &m_context;
    cgmt_start_cmd(ctx, 1, MfVersion);

    cgmt_int(ctx, value);

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::metafileDescription(char const *value)
{
    cgm_context *ctx = &m_context;
    cgmt_start_cmd(ctx, 1, MfDescrip);

    cgmt_string(ctx, value, strlen(value));

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::vdcType(VdcType type)
{
    cgm_context *ctx = &m_context;
    cgmt_start_cmd(ctx, 1, (int) cgm_class_1::vdcType);

    if (type == VdcType::Integer)
    {
        cgmt_out_string(ctx, " Integer");
    }
    else if (type == VdcType::Real)
    {
        cgmt_out_string(ctx, " Real");
    }

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::intPrecisionClearText(int min, int max)
{
    cgm_context *ctx = &m_context;
    cgmt_start_cmd(ctx, 1, (int) IntPrec);

    cgmt_int(ctx, min);
    cgmt_int(ctx, max);

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::intPrecisionBinary(int value)
{
    throw std::runtime_error("Unsupported method");
}

void ClearTextMetafileWriter::realPrecisionClearText(float minReal, float maxReal, int digits)
{
    cgm_context *ctx = &m_context;
    cgmt_start_cmd(ctx, 1, (int) RealPrec);

    cgmt_real(ctx, static_cast<double>(minReal));
    cgmt_real(ctx, static_cast<double>(maxReal));
    cgmt_int(ctx, digits);

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::realPrecisionBinary(RealPrecision prec, int expWidth, int mantWidth)
{
    throw std::runtime_error("Unsupported method");
}

void ClearTextMetafileWriter::indexPrecisionClearText(int min, int max)
{
    cgm_context *ctx = &m_context;
    cgmt_start_cmd(ctx, 1, (int) IndexPrec);

    cgmt_int(ctx, min);
    cgmt_int(ctx, max);

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::indexPrecisionBinary(int value)
{
    throw std::runtime_error("Unsupported method");
}

void ClearTextMetafileWriter::colorPrecisionClearText(int max)
{
    cgm_context *ctx = &m_context;
    cgmt_start_cmd(ctx, 1, (int) ColPrec);

    cgmt_int(ctx, max);

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::colorPrecisionBinary(int value)
{
    throw std::runtime_error("Unsupported method");
}

void ClearTextMetafileWriter::colorIndexPrecisionClearText(int max)
{
    cgm_context *ctx = &m_context;
    cgmt_start_cmd(ctx, 1, (int) CIndPrec);

    cgmt_int(ctx, max);

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::colorIndexPrecisionBinary(int value)
{
    throw std::runtime_error("Unsupported method");
}

void ClearTextMetafileWriter::maximumColorIndex(int max)
{
    cgm_context *ctx = &m_context;
    cgmt_start_cmd(ctx, 1, (int) MaxCInd);

    cgmt_int(ctx, max);

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::colorValueExtent(int redMin, int redMax, int greenMin, int greenMax, int blueMin, int blueMax)
{
    cgm_context *ctx = &m_context;
    cgmt_start_cmd(ctx, 1, (int) CVExtent);

    cgmt_int(ctx, redMin);
    cgmt_int(ctx, greenMin);
    cgmt_int(ctx, blueMin);
    cgmt_int(ctx, redMax);
    cgmt_int(ctx, greenMax);
    cgmt_int(ctx, blueMax);

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::metafileElementList()
{
    cgm_context *ctx = &m_context;

    cgmt_start_cmd(ctx, 1, (int) MfElList);

    cgmt_outc(ctx, ' ');
    cgmt_outc(ctx, quote_char);

    for (int i = 2; i < 2 * n_melements; i += 2)
    {
        cgmt_out_string(ctx, cgmt_cptr[element_list[i]][element_list[i + 1]]);

        if (i < 2 * (n_melements - 1))
            cgmt_outc(ctx, ' ');
    }

    cgmt_outc(ctx, quote_char);
    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::fontList(std::vector<std::string> const &fonts)
{
    cgm_context *ctx = &m_context;
    char s[max_str];

    cgmt_start_cmd(ctx, 1, (int) FontList);

    cgmt_outc(ctx, ' ');

    const int numFonts = static_cast<int>(fonts.size());
    for (int i = 0; i < numFonts; i++)
    {
        sprintf(s, "'%s'%s", fonts[i].c_str(), (i < numFonts - 1) ? ", " : "");
        cgmt_out_string(ctx, s);
    }

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::characterCodingAnnouncer(CharCodeAnnouncer value)
{
    cgm_context *ctx = &m_context;
    static const char *const announcerNames[] = {
        "Basic7Bit",
        "Basic8Bit",
        "Extd7Bit",
        "Extd8Bit"
    };
    cgmt_start_cmd(ctx, 1, (int) CharAnnounce);

    cgmt_outc(ctx, ' ');
    cgmt_out_string(ctx, announcerNames[static_cast<int>(value)]);

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::scalingMode(ScalingMode mode, float value)
{
    cgm_context *ctx = &m_context;
    cgmt_start_cmd(ctx, 2, (int) ScalMode);

    cgmt_out_string(ctx, mode == ScalingMode::Metric ? " Metric" : " Abstract");
    cgmt_real(ctx, static_cast<double>(value));

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::colorSelectionMode(ColorMode mode)
{
    cgm_context *ctx = &m_context;
    cgmt_start_cmd(ctx, 2, (int) ColSelMode);

    cgmt_out_string(ctx, mode == ColorMode::Indexed ? " Indexed" : " Direct");

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::lineWidthSpecificationMode(SpecificationMode mode)
{
    cgm_context *ctx = &m_context;
    cgmt_start_cmd(ctx, 2, (int) LWidSpecMode);

    cgmt_out_string(ctx, mode == SpecificationMode::Absolute ? " Absolute" : " Scaled");

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::markerSizeSpecificationMode(SpecificationMode mode)
{
    cgm_context *ctx = &m_context;
    cgmt_start_cmd(ctx, 2, (int) MarkSizSpecMode);

    cgmt_out_string(ctx, mode == SpecificationMode::Absolute ? " Absolute" : " Scaled");

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::vdcExtent(int llx, int lly, int urx, int ury)
{
    cgm_context *ctx = &m_context;
    cgmt_start_cmd(ctx, 2, (int) cgm_class_2::vdcExtent);

    cgmt_ipoint(ctx, llx, lly);
    cgmt_ipoint(ctx, urx, ury);

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::backgroundColor(int red, int green, int blue)
{
    cgm_context *ctx = &m_context;
    cgmt_start_cmd(ctx, 2, (int) BackCol);

    cgmt_int(ctx, red);
    cgmt_int(ctx, green);
    cgmt_int(ctx, blue);

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::vdcIntegerPrecisionClearText(int min, int max)
{
    cgm_context *ctx = &m_context;
    cgmt_start_cmd(ctx, 3, (int) vdcIntPrec);

    cgmt_int(ctx, min);
    cgmt_int(ctx, max);

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::vdcIntegerPrecisionBinary(int value)
{
    throw std::runtime_error("Unsupported method");
}

void ClearTextMetafileWriter::clipRectangle(int llx, int lly, int urx, int ury)
{
    cgm_context *ctx = &m_context;
    cgmt_start_cmd(ctx, 3, (int) ClipRect);

    cgmt_ipoint(ctx, llx, lly);
    cgmt_ipoint(ctx, urx, ury);

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::clipIndicator(bool enabled)
{
    cgm_context *ctx = &m_context;
    cgmt_start_cmd(ctx, 3, (int) ClipIndic);

    cgmt_out_string(ctx, enabled ? " On" : " Off");

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::polyline(const std::vector<Point<int>> &points)
{
    cgm_context *ctx = &m_context;

    cgmt_start_cmd(ctx, 4, (int) PolyLine);

    for (const Point<int> &point : points)
    {
        cgmt_ipoint(ctx, point.x, point.y);
    }

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::polymarker(const std::vector<Point<int>> &points)
{
    cgm_context *ctx = &m_context;

    cgmt_start_cmd(ctx, 4, (int) PolyMarker);

    for (const Point<int> &i : points)
    {
        cgmt_ipoint(ctx, i.x, i.y);
    }

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::text(Point<int> point, TextFlag flag, const char *text)
{
    cgm_context *ctx = &m_context;
    cgmt_start_cmd(ctx, 4, (int) Text);

    cgmt_ipoint(ctx, point.x, point.y);

    cgmt_out_string(ctx, flag == TextFlag::Final ? " Final" : " NotFinal");

    cgmt_string(ctx, text, strlen(text));

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::polygon(const std::vector<Point<int>> &points)
{
    cgm_context *ctx = &m_context;

    cgmt_start_cmd(ctx, 4, (int) C_Polygon);

    for (const Point<int> &i : points)
    {
        cgmt_ipoint(ctx, i.x, i.y);
    }

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::cellArray(Point<int> c1, Point<int> c2, Point<int> c3, int colorPrecision, int nx, int ny, const int *colors)
{
    cgm_context *ctx = &m_context;

    cgmt_start_cmd(ctx, 4, (int) Cell_Array);

    cgmt_ipoint(ctx, c1.x, c1.y);
    cgmt_ipoint(ctx, c2.x, c2.y);
    cgmt_ipoint(ctx, c3.x, c3.y);
    cgmt_int(ctx, nx);
    cgmt_int(ctx, ny);
    cgmt_int(ctx, colorPrecision);

    for (int iy = 0; iy < ny; iy++)
    {
        cgmt_fb(ctx);

        for (int ix = 0; ix < nx; ix++)
        {
            const int c = colors[nx * iy + ix];
            cgmt_int(ctx, c);

            if (ix < nx - 1)
                cgmt_outc(ctx, ',');
        }
    }

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::lineType(int value)
{
    cgm_context *ctx = &m_context;
    cgmt_start_cmd(ctx, 5, (int) LType);

    cgmt_int(ctx, (int) value);

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::lineWidth(float value)
{
    cgm_context *ctx = &m_context;
    cgmt_start_cmd(ctx, 5, (int) LWidth);

    cgmt_real(ctx, static_cast<double>(value));

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::lineColor(int value)
{
    cgm_context *ctx = &m_context;
    cgmt_start_cmd(ctx, 5, (int) LColour);

    cgmt_int(ctx, value);

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::markerType(int value)
{
    cgm_context *ctx = &m_context;
    cgmt_start_cmd(ctx, 5, (int) MType);

    cgmt_int(ctx, value);

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::markerSize(float value)
{
    cgm_context *ctx = &m_context;
    cgmt_start_cmd(ctx, 5, (int) MSize);

    cgmt_real(ctx, static_cast<double>(value));

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::markerColor(int value)
{
    cgm_context *ctx = &m_context;
    cgmt_start_cmd(ctx, 5, (int) MColour);

    cgmt_int(ctx, value);

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::textFontIndex(int value)
{
    cgm_context *ctx = &m_context;
    cgmt_start_cmd(ctx, 5, (int) TFIndex);

    cgmt_int(ctx, value);

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::textPrecision(TextPrecision value)
{
    cgm_context *ctx = &m_context;
    cgmt_start_cmd(ctx, 5, (int) TPrec);

    switch (static_cast<int>(value))
    {
    case string:
        cgmt_out_string(ctx, " String");
        break;

    case character:
        cgmt_out_string(ctx, " Character");
        break;

    case stroke:
        cgmt_out_string(ctx, " Stroke");
        break;
    }

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::charExpansion(float value)
{
    cgm_context *ctx = &m_context;
    cgmt_start_cmd(ctx, 5, (int) CExpFac);

    cgmt_real(ctx, static_cast<double>(value));

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::charSpacing(float value)
{
    cgm_context *ctx = &m_context;
    cgmt_start_cmd(ctx, 5, (int) CSpace);

    cgmt_real(ctx, static_cast<double>(value));

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::textColor(int index)
{
    cgm_context *ctx = &m_context;
    cgmt_start_cmd(ctx, 5, (int) TColour);

    cgmt_int(ctx, index);

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::charHeight(int value)
{
    cgm_context *ctx = &m_context;
    cgmt_start_cmd(ctx, 5, (int) CHeight);

    cgmt_int(ctx, value);

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::charOrientation(int upX, int upY, int baseX, int baseY)
{
    cgm_context *ctx = &m_context;
    cgmt_start_cmd(ctx, 5, (int) COrient);

    cgmt_int(ctx, upX);
    cgmt_int(ctx, upY);
    cgmt_int(ctx, baseX);
    cgmt_int(ctx, baseY);

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::textPath(TextPath value)
{
    cgm_context *ctx = &m_context;
    cgmt_start_cmd(ctx, 5, (int) TPath);

    switch (static_cast<int>(value))
    {
    case right:
        cgmt_out_string(ctx, " Right");
        break;

    case left:
        cgmt_out_string(ctx, " Left");
        break;

    case up:
        cgmt_out_string(ctx, " Up");
        break;

    case down:
        cgmt_out_string(ctx, " Down");
        break;
    }

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::textAlignment(HorizAlign horiz, VertAlign vert, float contHoriz, float contVert)
{
    cgm_context *ctx = &m_context;
    cgmt_start_cmd(ctx, 5, (int) TAlign);

    switch (static_cast<int>(horiz))
    {
    case normal_h:
        cgmt_out_string(ctx, " NormHoriz");
        break;

    case left_h:
        cgmt_out_string(ctx, " Left");
        break;

    case center_h:
        cgmt_out_string(ctx, " Ctr");
        break;

    case right_h:
        cgmt_out_string(ctx, " Right");
        break;

    case cont_h:
        cgmt_out_string(ctx, " ContHoriz");
        break;
    }

    switch (static_cast<int>(vert))
    {
    case normal_v:
        cgmt_out_string(ctx, " NormVert");
        break;

    case top_v:
        cgmt_out_string(ctx, " Top");
        break;

    case cap_v:
        cgmt_out_string(ctx, " Cap");
        break;

    case half_v:
        cgmt_out_string(ctx, " Half");
        break;

    case base_v:
        cgmt_out_string(ctx, " Base");
        break;

    case bottom_v:
        cgmt_out_string(ctx, " Bottom");
        break;

    case cont_v:
        cgmt_out_string(ctx, " ContVert");
        break;
    }

    cgmt_real(ctx, static_cast<double>(contHoriz));
    cgmt_real(ctx, static_cast<double>(contVert));

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::interiorStyle(InteriorStyle value)
{
    cgm_context *ctx = &m_context;
    cgmt_start_cmd(ctx, 5, (int) IntStyle);

    switch (static_cast<int>(value))
    {
    case hollow:
        cgmt_out_string(ctx, " Hollow");
        break;

    case solid_i:
        cgmt_out_string(ctx, " Solid");
        break;

    case pattern:
        cgmt_out_string(ctx, " Pat");
        break;

    case hatch:
        cgmt_out_string(ctx, " Hatch");
        break;

    case empty:
        cgmt_out_string(ctx, " Empty");
        break;
    }

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::fillColor(int value)
{
    cgm_context *ctx = &m_context;
    cgmt_start_cmd(ctx, 5, (int) FillColour);

    cgmt_int(ctx, value);

    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::hatchIndex(int value)
{
    cgm_context *ctx = &m_context;
    cgmt_start_cmd(ctx, 5, (int) HatchIndex);
    cgmt_int(ctx, value);
    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::patternIndex(int value)
{
    cgm_context *ctx = &m_context;
    cgmt_start_cmd(ctx, 5, (int) PatIndex);
    cgmt_int(ctx, value);
    cgmt_flush_cmd(ctx, final_flush);
}

void ClearTextMetafileWriter::colorTable(int startIndex, std::vector<Color> const &colorTable)
{
    cgm_context *ctx = &m_context;

    cgmt_start_cmd(ctx, 5, (int) ColTab);
    cgmt_int(ctx, startIndex);

    for (int i = startIndex; i < (startIndex + static_cast<int>(colorTable.size())); ++i)
    {
        cgmt_int(ctx, (int) (colorTable[(i - startIndex)].red * (max_colors - 1)));
        cgmt_int(ctx, (int) (colorTable[(i - startIndex)].green * (max_colors - 1)));
        cgmt_int(ctx, (int) (colorTable[(i - startIndex)].blue * (max_colors - 1)));
    }

    cgmt_flush_cmd(ctx, final_flush);
}

}        // namespace cgm
