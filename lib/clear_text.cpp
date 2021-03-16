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

/* Write a real variable */
static void cgmt_real(cgm_context *ctx, double xin)
{
    char buffer[max_str];

    sprintf(buffer, " %.6f", xin);
    cgmt_out_string(ctx, buffer);
}

/* Write an integer point */
static void cgmt_ipoint(cgm_context *ctx, int x, int y)
{
    char buffer[max_str];

    sprintf(buffer, " %d,%d", x, y);
    cgmt_out_string(ctx, buffer);
}

/* Begin metafile */
static void cgmt_begin_p(cgm_context *p, const char *comment)
{
    cgmt_start_cmd(p, 0, (int) B_Mf);

    if (*comment)
        cgmt_string(p, comment, static_cast<int>(strlen(comment)));
    else
        cgmt_string(p, nullptr, 0);

    cgmt_flush_cmd(p, final_flush);
}

/* End metafile */
static void cgmt_end_p(cgm_context *ctx)
{
    cgmt_start_cmd(ctx, 0, (int) E_Mf);

    cgmt_flush_cmd(ctx, final_flush);

    cgmt_fb(ctx);
}

/* Begin picture */
static void cgmt_bp_p(cgm_context *ctx, const char *pic_name)
{
    cgmt_start_cmd(ctx, 0, (int) B_Pic);

    if (*pic_name)
        cgmt_string(ctx, pic_name, strlen(pic_name));
    else
        cgmt_string(ctx, NULL, 0);

    cgmt_flush_cmd(ctx, final_flush);
}

/* Begin picture body */
static void cgmt_bpage_p(cgm_context *ctx)
{
    cgmt_start_cmd(ctx, 0, (int) B_Pic_Body);

    cgmt_flush_cmd(ctx, final_flush);
}

/* End picture */
static void cgmt_epage_p(cgm_context *ctx)
{
    cgmt_start_cmd(ctx, 0, (int) E_Pic);

    cgmt_flush_cmd(ctx, final_flush);
}

/* Metafile version */
static void cgmt_mfversion_p(cgm_context *ctx, int version)
{
    cgmt_start_cmd(ctx, 1, version);

    cgmt_int(ctx, 1);

    cgmt_flush_cmd(ctx, final_flush);
}

/* Metafile description */
static void cgmt_mfdescrip_p(cgm_context *ctx, const char *descrip)
{
    cgmt_start_cmd(ctx, 1, MfDescrip);

    cgmt_string(ctx, descrip, strlen(descrip));

    cgmt_flush_cmd(ctx, final_flush);
}

/* VDC type */
static void cgmt_vdctype_p(cgm_context *ctx, cgm::VdcType value)
{
    cgmt_start_cmd(ctx, 1, (int) vdcType);

    if (value == cgm::VdcType::Integer)
    {
        cgmt_out_string(ctx, " Integer");
    }
    else if (value == cgm::VdcType::Real)
    {
        cgmt_out_string(ctx, " Real");
    }

    cgmt_flush_cmd(ctx, final_flush);
}

/* Integer precision */
static void cgmt_intprec_p(cgm_context *ctx, int min, int max)
{
    cgmt_start_cmd(ctx, 1, (int) IntPrec);

    cgmt_int(ctx, min);
    cgmt_int(ctx, max);

    cgmt_flush_cmd(ctx, final_flush);
}

/* Real precision */
static void cgmt_realprec_p(cgm_context *ctx, double minReal, double maxReal, int digits)
{
    cgmt_start_cmd(ctx, 1, (int) RealPrec);

    cgmt_real(ctx, minReal);
    cgmt_real(ctx, maxReal);
    cgmt_int(ctx, digits);

    cgmt_flush_cmd(ctx, final_flush);
}

/* Index precision */
static void cgmt_indexprec_p(cgm_context *ctx, int min, int max)
{
    cgmt_start_cmd(ctx, 1, (int) IndexPrec);

    cgmt_int(ctx, min);
    cgmt_int(ctx, max);

    cgmt_flush_cmd(ctx, final_flush);
}

/* Colour precision */
static void cgmt_colprec_p(cgm_context *ctx, int max)
{
    cgmt_start_cmd(ctx, 1, (int) ColPrec);

    cgmt_int(ctx, max);

    cgmt_flush_cmd(ctx, final_flush);
}

/* Colour index precision */
static void cgmt_cindprec_p(cgm_context *ctx, int max)
{
    cgmt_start_cmd(ctx, 1, (int) CIndPrec);

    cgmt_int(ctx, max);

    cgmt_flush_cmd(ctx, final_flush);
}

/* Colour value extent */
static void cgmt_cvextent_p(cgm_context *ctx, int minRed, int maxRed, int minGreen, int maxGreen, int minBlue, int maxBlue)
{
    cgmt_start_cmd(ctx, 1, (int) CVExtent);

    cgmt_int(ctx, minRed);
    cgmt_int(ctx, minGreen);
    cgmt_int(ctx, minBlue);
    cgmt_int(ctx, maxRed);
    cgmt_int(ctx, maxGreen);
    cgmt_int(ctx, maxBlue);

    cgmt_flush_cmd(ctx, final_flush);
}

/* Maximum colour index */
static void cgmt_maxcind_p(cgm_context *ctx, int max)
{
    cgmt_start_cmd(ctx, 1, (int) MaxCInd);

    cgmt_int(ctx, max);

    cgmt_flush_cmd(ctx, final_flush);
}

/* Metafile element list */
static void cgmt_mfellist_p(cgm_context *ctx)
{
    int i;

    cgmt_start_cmd(ctx, 1, (int) MfElList);

    cgmt_outc(ctx, ' ');
    cgmt_outc(ctx, quote_char);

    for (i = 2; i < 2 * n_melements; i += 2)
    {
        cgmt_out_string(ctx, cgmt_cptr[element_list[i]][element_list[i + 1]]);

        if (i < 2 * (n_melements - 1))
            cgmt_outc(ctx, ' ');
    }

    cgmt_outc(ctx, quote_char);
    cgmt_flush_cmd(ctx, final_flush);
}

/* Font List */
static void cgmt_fontlist_p(cgm_context *ctx, int numFonts, const char **fonts)
{
    int i;
    char s[max_str];

    cgmt_start_cmd(ctx, 1, (int) FontList);

    cgmt_outc(ctx, ' ');

    for (i = 0; i < numFonts; i++)
    {
        sprintf(s, "'%s'%s", fonts[i], (i < numFonts - 1) ? ", " : "");
        cgmt_out_string(ctx, s);
    }

    cgmt_flush_cmd(ctx, final_flush);
}

/* Character announcer */
static void cgmt_cannounce_p(cgm_context *ctx, int value)
{
    const char *announcerNames[] = {
        "Basic7Bit",
        "Basic8Bit",
        "Extd7Bit",
        "Extd8Bit"
    };
    cgmt_start_cmd(ctx, 1, (int) CharAnnounce);

    cgmt_outc(ctx, ' ');
    cgmt_out_string(ctx, announcerNames[value]);

    cgmt_flush_cmd(ctx, final_flush);
}

/* Scaling mode */
static void cgmt_scalmode_p(cgm_context *ctx, int mode, double value)
{
    cgmt_start_cmd(ctx, 2, (int) ScalMode);

    if (mode == 1)
    {
        cgmt_out_string(ctx, " Metric");
    }
    else
    {
        cgmt_out_string(ctx, " Abstract");
    }
    cgmt_real(ctx, value);

    cgmt_flush_cmd(ctx, final_flush);
}

/* Colour selection mode */
static void cgmt_colselmode_p(cgm_context *ctx, int mode)
{
    cgmt_start_cmd(ctx, 2, (int) ColSelMode);

    cgmt_out_string(ctx, mode == 0 ? " Indexed" : " Direct");

    cgmt_flush_cmd(ctx, final_flush);
}

/* Line width specification mode */
static void cgmt_lwsmode_p(cgm_context *ctx, int mode)
{
    cgmt_start_cmd(ctx, 2, (int) LWidSpecMode);

    cgmt_out_string(ctx, mode == 0 ? " Absolute" : " Scaled");

    cgmt_flush_cmd(ctx, final_flush);
}

/* Marker size specification mode */
static void cgmt_msmode_p(cgm_context *ctx, int mode)
{
    cgmt_start_cmd(ctx, 2, (int) MarkSizSpecMode);

    cgmt_out_string(ctx, mode == 0 ? " Absolute" : " Scaled");

    cgmt_flush_cmd(ctx, final_flush);
}

/* VDC extent */
static void cgmt_vdcextent_p(cgm_context *ctx, int llx, int lly, int urx, int ury)
{
    cgmt_start_cmd(ctx, 2, (int) vdcExtent);

    cgmt_ipoint(ctx, llx, lly);
    cgmt_ipoint(ctx, urx, ury);

    cgmt_flush_cmd(ctx, final_flush);
}

/* Background colour */
static void cgmt_backcol_p(cgm_context *ctx, int red, int green, int blue)
{
    cgmt_start_cmd(ctx, 2, (int) BackCol);

    cgmt_int(ctx, red);
    cgmt_int(ctx, green);
    cgmt_int(ctx, blue);

    cgmt_flush_cmd(ctx, final_flush);
}

/* VDC integer precision */
static void cgmt_vdcintprec_p(cgm_context *ctx, int min, int max)
{
    cgmt_start_cmd(ctx, 3, (int) vdcIntPrec);

    cgmt_int(ctx, min);
    cgmt_int(ctx, max);

    cgmt_flush_cmd(ctx, final_flush);
}

/* Clip rectangle */
static void cgmt_cliprect_p(cgm_context *ctx, int llx, int lly, int urx, int ury)
{
    cgmt_start_cmd(ctx, 3, (int) ClipRect);

    cgmt_ipoint(ctx, llx, lly);
    cgmt_ipoint(ctx, urx, ury);

    cgmt_flush_cmd(ctx, final_flush);
}

/* Clip indicator */
static void cgmt_clipindic_p(cgm_context *ctx, bool clip_ind)
{
    cgmt_start_cmd(ctx, 3, (int) ClipIndic);

    cgmt_out_string(ctx, clip_ind ? " On" : " Off");

    cgmt_flush_cmd(ctx, final_flush);
}

/* Polyline */
static void cgmt_pline_pt(cgm_context *ctx, int numPoints, const cgm::Point<int> *points)
{
    int i;

    cgmt_start_cmd(ctx, 4, (int) PolyLine);

    for (i = 0; i < numPoints; ++i)
    {
        cgmt_ipoint(ctx, points[i].x, points[i].y);
    }

    cgmt_flush_cmd(ctx, final_flush);
}

/* Polymarker */
static void cgmt_pmarker_pt(cgm_context *ctx, int numPoints, const cgm::Point<int> *points)
{
    int i;

    cgmt_start_cmd(ctx, 4, (int) PolyMarker);

    for (i = 0; i < numPoints; ++i)
    {
        cgmt_ipoint(ctx, points[i].x, points[i].y);
    }

    cgmt_flush_cmd(ctx, final_flush);
}

/* Text */
static void cgmt_text_p(cgm_context *ctx, int x, int y, bool final, const char *buffer)
{
    cgmt_start_cmd(ctx, 4, (int) Text);

    cgmt_ipoint(ctx, x, y);

    cgmt_out_string(ctx, final ? " Final" : " NotFinal");

    cgmt_string(ctx, buffer, strlen(buffer));

    cgmt_flush_cmd(ctx, final_flush);
}

/* Polygon */
static void cgmt_pgon_pt(cgm_context *ctx, int no_pairs, const cgm::Point<int> *points)
{
    int i;

    cgmt_start_cmd(ctx, 4, (int) C_Polygon);

    for (i = 0; i < no_pairs; ++i)
    {
        cgmt_ipoint(ctx, points[i].x, points[i].y);
    }

    cgmt_flush_cmd(ctx, final_flush);
}

/* Line type */
static void cgmt_ltype_p(cgm_context *ctx, int line_type)
{
    cgmt_start_cmd(ctx, 5, (int) LType);

    cgmt_int(ctx, (int) line_type);

    cgmt_flush_cmd(ctx, final_flush);
}

/* Line width */
static void cgmt_lwidth_p(cgm_context *ctx, double rmul)
{
    cgmt_start_cmd(ctx, 5, (int) LWidth);

    cgmt_real(ctx, rmul);

    cgmt_flush_cmd(ctx, final_flush);
}

/* Line colour */
static void cgmt_lcolor_p(cgm_context *ctx, int index)
{
    cgmt_start_cmd(ctx, 5, (int) LColour);

    cgmt_int(ctx, index);

    cgmt_flush_cmd(ctx, final_flush);
}

/* Marker type */
static void cgmt_mtype_p(cgm_context *ctx, int marker)
{
    cgmt_start_cmd(ctx, 5, (int) MType);

    cgmt_int(ctx, marker);

    cgmt_flush_cmd(ctx, final_flush);
}

/* Marker size */
static void cgmt_msize_p(cgm_context *ctx, double rmul)
{
    cgmt_start_cmd(ctx, 5, (int) MSize);

    cgmt_real(ctx, rmul);

    cgmt_flush_cmd(ctx, final_flush);
}

/* Marker colour */
static void cgmt_mcolor_p(cgm_context *ctx, int index)
{
    cgmt_start_cmd(ctx, 5, (int) MColour);

    cgmt_int(ctx, index);

    cgmt_flush_cmd(ctx, final_flush);
}

/* Text font index */
static void cgmt_tfindex_p(cgm_context *ctx, int index)
{
    cgmt_start_cmd(ctx, 5, (int) TFIndex);

    cgmt_int(ctx, index);

    cgmt_flush_cmd(ctx, final_flush);
}

/* Text precision */
static void cgmt_tprec_p(cgm_context *ctx, int precision)
{
    cgmt_start_cmd(ctx, 5, (int) TPrec);

    switch (precision)
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

/* Character expansion factor */
static void cgmt_cexpfac_p(cgm_context *ctx, double factor)
{
    cgmt_start_cmd(ctx, 5, (int) CExpFac);

    cgmt_real(ctx, factor);

    cgmt_flush_cmd(ctx, final_flush);
}

/* Character space */
static void cgmt_cspace_p(cgm_context *ctx, double space)
{
    cgmt_start_cmd(ctx, 5, (int) CSpace);

    cgmt_real(ctx, space);

    cgmt_flush_cmd(ctx, final_flush);
}

/* Cell array */
static void cgmt_carray_p(cgm_context *ctx, int c1x, int c1y, int c2x, int c2y, int c3x, int c3y, int colorPrecision, int nx, int ny, int dimx, const int *array)
{
    int ix, iy, c;

    cgmt_start_cmd(ctx, 4, (int) Cell_Array);

    cgmt_ipoint(ctx, c1x, c1y);
    cgmt_ipoint(ctx, c2x, c2y);
    cgmt_ipoint(ctx, c3x, c3y);
    cgmt_int(ctx, nx);
    cgmt_int(ctx, ny);
    cgmt_int(ctx, colorPrecision);

    for (iy = 0; iy < ny; iy++)
    {
        cgmt_fb(ctx);

        for (ix = 0; ix < nx; ix++)
        {
            c = array[dimx * iy + ix];
            cgmt_int(ctx, c);

            if (ix < nx - 1)
                cgmt_outc(ctx, ',');
        }
    }

    cgmt_flush_cmd(ctx, final_flush);
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


void ClearTextMetafileWriter::beginMetafile(const char *identifier)
{
    cgmt_begin_p(&m_context, identifier);
}

void ClearTextMetafileWriter::endMetafile()
{
    cgmt_end_p(&m_context);
}

void ClearTextMetafileWriter::beginPicture(char const *identifier)
{
    cgmt_bp_p(&m_context, identifier);
}

void ClearTextMetafileWriter::beginPictureBody()
{
    cgmt_bpage_p(&m_context);
}

void ClearTextMetafileWriter::endPicture()
{
    cgmt_epage_p(&m_context);
}

void ClearTextMetafileWriter::metafileVersion(int value)
{
    cgmt_mfversion_p(&m_context, value);
}

void ClearTextMetafileWriter::metafileDescription(char const *value)
{
    cgmt_mfdescrip_p(&m_context, value);
}

void ClearTextMetafileWriter::vdcType(VdcType type)
{
    cgmt_vdctype_p(&m_context, type);
}

void ClearTextMetafileWriter::intPrecisionClearText(int min, int max)
{
    cgmt_intprec_p(&m_context, min, max);
}

void ClearTextMetafileWriter::intPrecisionBinary(int value)
{
    throw std::runtime_error("Unsupported method");
}

void ClearTextMetafileWriter::realPrecisionClearText(float minReal, float maxReal, int digits)
{
    cgmt_realprec_p(&m_context, static_cast<double>(minReal), static_cast<double>(maxReal), digits);
}

void ClearTextMetafileWriter::realPrecisionBinary(RealPrecision prec, int expWidth, int mantWidth)
{
    throw std::runtime_error("Unsupported method");
}

void ClearTextMetafileWriter::indexPrecisionClearText(int min, int max)
{
    cgmt_indexprec_p(&m_context, min, max);
}

void ClearTextMetafileWriter::indexPrecisionBinary(int value)
{
    throw std::runtime_error("Unsupported method");
}

void ClearTextMetafileWriter::colorPrecisionClearText(int max)
{
    cgmt_colprec_p(&m_context, max);
}

void ClearTextMetafileWriter::colorPrecisionBinary(int value)
{
    throw std::runtime_error("Unsupported method");
}

void ClearTextMetafileWriter::colorIndexPrecisionClearText(int max)
{
    cgmt_cindprec_p(&m_context, max);
}

void ClearTextMetafileWriter::colorIndexPrecisionBinary(int value)
{
    throw std::runtime_error("Unsupported method");
}

void ClearTextMetafileWriter::maximumColorIndex(int max)
{
    cgmt_maxcind_p(&m_context, max);
}

void ClearTextMetafileWriter::colorValueExtent(int redMin, int redMax, int greenMin, int greenMax, int blueMin, int blueMax)
{
    cgmt_cvextent_p(&m_context, redMin, redMax, greenMin, greenMax, blueMin, blueMax);
}

void ClearTextMetafileWriter::metafileElementList()
{
    cgmt_mfellist_p(&m_context);
}

void ClearTextMetafileWriter::fontList(std::vector<std::string> const &fonts)
{
    std::vector<const char *> fontNames;
    fontNames.reserve(fonts.size());
    for (const std::string &font : fonts)
    {
        fontNames.push_back(font.c_str());
    }
    cgmt_fontlist_p(&m_context, static_cast<int>(fonts.size()), fontNames.data());
}

void ClearTextMetafileWriter::characterCodingAnnouncer(CharCodeAnnouncer value)
{
    cgmt_cannounce_p(&m_context, static_cast<int>(value));
}

void ClearTextMetafileWriter::scalingMode(ScalingMode mode, float value)
{
    cgmt_scalmode_p(&m_context, static_cast<int>(mode), static_cast<double>(value));
}

void ClearTextMetafileWriter::colorSelectionMode(ColorMode mode)
{
    cgmt_colselmode_p(&m_context, static_cast<int>(mode));
}

void ClearTextMetafileWriter::lineWidthSpecificationMode(SpecificationMode mode)
{
    cgmt_lwsmode_p(&m_context, static_cast<int>(mode));
}

void ClearTextMetafileWriter::markerSizeSpecificationMode(SpecificationMode mode)
{
    cgmt_msmode_p(&m_context, static_cast<int>(mode));
}

void ClearTextMetafileWriter::vdcExtent(int llx, int lly, int urx, int ury)
{
    cgmt_vdcextent_p(&m_context, llx, lly, urx, ury);
}

void ClearTextMetafileWriter::backgroundColor(int red, int green, int blue)
{
    cgmt_backcol_p(&m_context, red, green, blue);
}

void ClearTextMetafileWriter::vdcIntegerPrecisionClearText(int min, int max)
{
    cgmt_vdcintprec_p(&m_context, min, max);
}

void ClearTextMetafileWriter::vdcIntegerPrecisionBinary(int value)
{
    throw std::runtime_error("Unsupported method");
}

void ClearTextMetafileWriter::clipRectangle(int llx, int lly, int urx, int ury)
{
    cgmt_cliprect_p(&m_context, llx, lly, urx, ury);
}

void ClearTextMetafileWriter::clipIndicator(bool enabled)
{
    cgmt_clipindic_p(&m_context, enabled);
}

void ClearTextMetafileWriter::polyline(const std::vector<Point<int>> &points)
{
    cgmt_pline_pt(&m_context, static_cast<int>(points.size()), points.data());
}

void ClearTextMetafileWriter::polymarker(const std::vector<Point<int>> &points)
{
    cgmt_pmarker_pt(&m_context, static_cast<int>(points.size()), points.data());
}

void ClearTextMetafileWriter::text(Point<int> point, TextFlag flag, const char *text)
{
    cgmt_text_p(&m_context, point.x, point.y, static_cast<int>(flag), text);
}

void ClearTextMetafileWriter::polygon(const std::vector<Point<int>> &points)
{
    cgmt_pgon_pt(&m_context, static_cast<int>(points.size()), points.data());
}

void ClearTextMetafileWriter::cellArray(Point<int> c1, Point<int> c2, Point<int> c3, int colorPrecision, int nx, int ny, const int *colors)
{
    cgmt_carray_p(&m_context, c1.x, c1.y, c2.x, c2.y, c3.x, c3.y, colorPrecision, nx, ny, nx, colors);
}

void ClearTextMetafileWriter::lineType(int value)
{
    cgmt_ltype_p(&m_context, value);
}

void ClearTextMetafileWriter::lineWidth(float value)
{
    cgmt_lwidth_p(&m_context, static_cast<double>(value));
}

void ClearTextMetafileWriter::lineColor(int value)
{
    cgmt_lcolor_p(&m_context, value);
}

void ClearTextMetafileWriter::markerType(int value)
{
    cgmt_mtype_p(&m_context, value);
}

void ClearTextMetafileWriter::markerSize(float value)
{
    cgmt_msize_p(&m_context, static_cast<double>(value));
}

void ClearTextMetafileWriter::markerColor(int value)
{
    cgmt_mcolor_p(&m_context, value);
}

void ClearTextMetafileWriter::textFontIndex(int value)
{
    cgmt_tfindex_p(&m_context, value);
}

void ClearTextMetafileWriter::textPrecision(TextPrecision value)
{
    cgmt_tprec_p(&m_context, static_cast<int>(value));
}

void ClearTextMetafileWriter::charExpansion(float value)
{
    cgmt_cexpfac_p(&m_context, static_cast<double>(value));
}

void ClearTextMetafileWriter::charSpacing(float value)
{
    cgmt_cspace_p(&m_context, static_cast<double>(value));
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
