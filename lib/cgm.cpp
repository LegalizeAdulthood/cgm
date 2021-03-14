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

#include "gkscore.h"
#include "impl.h"

#include "stream.h"

#define odd(number) ((number) &01)
#define nint(a) ((int) ((a) + 0.5))

static cgm_context *g_p;

static char digits[10] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };

static const char *fonts[max_std_textfont] = {
    "Times-Roman", "Times-Italic", "Times-Bold", "Times-BoldItalic",
    "Helvetica", "Helvetica-Oblique", "Helvetica-Bold", "Helvetica-BoldOblique",
    "Courier", "Courier-Oblique", "Courier-Bold", "Courier-BoldOblique",
    "Symbol",
    "Bookman-Light", "Bookman-LightItalic", "Bookman-Demi", "Bookman-DemiItalic",
    "NewCenturySchlbk-Roman", "NewCenturySchlbk-Italic", "NewCenturySchlbk-Bold", "NewCenturySchlbk-BoldItalic",
    "AvantGarde-Book", "AvantGarde-BookOblique", "AvantGarde-Demi", "AvantGarde-DemiOblique",
    "Palatino-Roman", "Palatino-Italic", "Palatino-Bold", "Palatino-BoldItalic"
};

static int map[] = {
    22, 9, 5, 14, 18, 26, 13, 1,
    24, 11, 7, 16, 20, 28, 13, 3,
    23, 10, 6, 15, 19, 27, 13, 2,
    25, 12, 8, 17, 21, 29, 13, 4
};

/* Transform world coordinates to virtual device coordinates */

static void WC_to_VDC(double xin, double yin, int *xout, int *yout)
{
    double x, y;

    /* Normalization transformation */

    x = g_p->xform.a * xin + g_p->xform.b;
    y = g_p->xform.c * yin + g_p->xform.d;

    /* Segment transformation */

    gks_seg_xform(&x, &y);

    /* Virtual device transformation */

    *xout = (int) (x * max_coord);
    *yout = (int) (y * max_coord);
}

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
static void cgmt_pline_p(cgm_context *ctx, int no_pairs, int *x1_ptr, int *y1_ptr)
{
    int i;

    cgmt_start_cmd(ctx, 4, (int) PolyLine);

    for (i = 0; i < no_pairs; ++i)
    {
        cgmt_ipoint(ctx, x1_ptr[i], y1_ptr[i]);
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
static void cgmt_pmarker_p(cgm_context *ctx, int no_pairs, int *x1_ptr, int *y1_ptr)
{
    int i;

    cgmt_start_cmd(ctx, 4, (int) PolyMarker);

    for (i = 0; i < no_pairs; ++i)
    {
        cgmt_ipoint(ctx, x1_ptr[i], y1_ptr[i]);
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
static void cgmt_pgon_p(cgm_context *ctx, int no_pairs, int *x1_ptr, int *y1_ptr)
{
    int i;

    cgmt_start_cmd(ctx, 4, (int) C_Polygon);

    for (i = 0; i < no_pairs; ++i)
    {
        cgmt_ipoint(ctx, x1_ptr[i], y1_ptr[i]);
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

/* Text colour */
static void cgmt_tcolor_p(cgm_context *ctx, int index)
{
    cgmt_start_cmd(ctx, 5, (int) TColour);

    cgmt_int(ctx, index);

    cgmt_flush_cmd(ctx, final_flush);
}

/* Character height */
static void cgmt_cheight_p(cgm_context *ctx, int height)
{
    cgmt_start_cmd(ctx, 5, (int) CHeight);

    cgmt_int(ctx, height);

    cgmt_flush_cmd(ctx, final_flush);
}

/* Character orientation */
static void cgmt_corient_p(cgm_context *ctx, int x_up, int y_up, int x_base, int y_base)
{
    cgmt_start_cmd(ctx, 5, (int) COrient);

    cgmt_int(ctx, x_up);
    cgmt_int(ctx, y_up);
    cgmt_int(ctx, x_base);
    cgmt_int(ctx, y_base);

    cgmt_flush_cmd(ctx, final_flush);
}

/* Text path */
static void cgmt_tpath_p(cgm_context *ctx, int new_path)
{
    cgmt_start_cmd(ctx, 5, (int) TPath);

    switch (new_path)
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

/* Text alignment */
static void cgmt_talign_p(cgm_context *ctx, int hor, int ver, double contHoriz, double contVert)
{
    cgmt_start_cmd(ctx, 5, (int) TAlign);

    switch (hor)
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

    switch (ver)
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

    cgmt_real(ctx, contHoriz);
    cgmt_real(ctx, contVert);

    cgmt_flush_cmd(ctx, final_flush);
}

/* Interior style */
static void cgmt_intstyle_p(cgm_context *ctx, int style)
{
    cgmt_start_cmd(ctx, 5, (int) IntStyle);

    switch (style)
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

/* Fill colour */
static void cgmt_fillcolor_p(cgm_context *ctx, int index)
{
    cgmt_start_cmd(ctx, 5, (int) FillColour);

    cgmt_int(ctx, index);

    cgmt_flush_cmd(ctx, final_flush);
}

/* Hatch index */
static void cgmt_hindex_p(cgm_context *ctx, int new_index)
{
    cgmt_start_cmd(ctx, 5, (int) HatchIndex);
    cgmt_int(ctx, new_index);
    cgmt_flush_cmd(ctx, final_flush);
}

/* Pattern index */
static void cgmt_pindex_p(cgm_context *ctx, int new_index)
{
    cgmt_start_cmd(ctx, 5, (int) PatIndex);
    cgmt_int(ctx, new_index);
    cgmt_flush_cmd(ctx, final_flush);
}

/* Colour table */
static void cgmt_coltab_c(cgm_context *ctx, int startIndex, int numColors, const cgm::Color *colors)
{
    int i, j;

    cgmt_start_cmd(ctx, 5, (int) ColTab);
    cgmt_int(ctx, startIndex);

    for (i = startIndex; i < (startIndex + numColors); ++i)
    {
        cgmt_int(ctx, (int) (colors[(i - startIndex)].red * (max_colors - 1)));
        cgmt_int(ctx, (int) (colors[(i - startIndex)].green * (max_colors - 1)));
        cgmt_int(ctx, (int) (colors[(i - startIndex)].blue * (max_colors - 1)));
    }

    cgmt_flush_cmd(ctx, final_flush);
}
static void cgmt_coltab_p(cgm_context *ctx, int beg_index, int no_entries, double *ctab)
{
    int i, j;

    cgmt_start_cmd(ctx, 5, (int) ColTab);
    cgmt_int(ctx, beg_index);

    for (i = beg_index; i < (beg_index + no_entries); ++i)
    {
        for (j = 0; j < 3; ++j)
        {
            cgmt_int(ctx, (int) (ctab[(i - beg_index) * 3 + j] * (max_colors - 1)));
        }
    }

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
            c = Color8Bit(c);
            cgmt_int(ctx, c);

            if (ix < nx - 1)
                cgmt_outc(ctx, ',');
        }
    }

    cgmt_flush_cmd(ctx, final_flush);
}

/* Flush output buffer */

static void cgmb_fb(cgm_context *ctx)
{
    if (ctx->buffer_ind != 0)
    {
        ctx->buffer[ctx->buffer_ind] = '\0';
        ctx->flush_buffer(ctx, ctx->flush_buffer_context);

        ctx->buffer_ind = 0;
        ctx->buffer[0] = '\0';
    }
}

/* Write one byte to buffer */

static void cgmb_outc(cgm_context *ctx, char chr)
{
    if (ctx->buffer_ind >= max_buffer)
        cgmb_fb(ctx);

    ctx->buffer[ctx->buffer_ind++] = chr;
}

/* Start output command */
static void cgmb_start_cmd(cgm_context *ctx, int cl, int el)
{
#define cl_max 15
#define el_max 127

    ctx->cmd_hdr = ctx->cmd_buffer + ctx->bfr_index;
    ctx->cmd_data = ctx->cmd_hdr + hdr_long;
    ctx->bfr_index += hdr_long;

    ctx->cmd_hdr[0] = static_cast<char>(cl << 4 | el >> 3);
    ctx->cmd_hdr[1] = static_cast<char>(el << 5);
    ctx->cmd_index = 0;
    ctx->partition = 1;

#undef cl_max
#undef el_max
}

/* Flush output command */
static void cgmb_flush_cmd(cgm_context *ctx, int this_flush)
{
    int i;

    if ((this_flush == final_flush) && (ctx->partition == 1) && (ctx->cmd_index <= max_short))
    {
        ctx->cmd_hdr[1] |= ctx->cmd_index;

        /* flush out the header */

        for (i = 0; i < hdr_short; ++i)
        {
            cgmb_outc(ctx, ctx->cmd_hdr[i]);
        }
    }
    else
    {
        /* need a long form */

        if (ctx->partition == 1)
        {
            /* first one */

            ctx->cmd_hdr[1] |= 31;

            for (i = 0; i < hdr_short; ++i)
            {
                cgmb_outc(ctx, ctx->cmd_hdr[i]);
            }
        }

        ctx->cmd_hdr[2] = ctx->cmd_index >> 8;
        ctx->cmd_hdr[3] = ctx->cmd_index & 255;

        if (this_flush == int_flush)
        {
            ctx->cmd_hdr[2] |= 1 << 7; /* more come */
        }

        /* flush out the header */

        for (i = hdr_short; i < hdr_long; ++i)
        {
            cgmb_outc(ctx, ctx->cmd_hdr[i]);
        }
    }

    /* now flush out the data */

    for (i = 0; i < ctx->cmd_index; ++i)
    {
        cgmb_outc(ctx, ctx->cmd_data[i]);
    }

    if (ctx->cmd_index % 2)
    {
        cgmb_outc(ctx, '\0');
    }

    ctx->cmd_index = 0;
    ctx->bfr_index = 0;
    ++ctx->partition;
}

/* Write one byte */
static void cgmb_out_bc(cgm_context *ctx, int c)
{
    if (ctx->cmd_index >= max_long)
    {
        cgmb_flush_cmd(ctx, int_flush);
    }

    ctx->cmd_data[ctx->cmd_index++] = c;
}

/* Write multiple bytes */
static void cgmb_out_bs(cgm_context *ctx, const char *cptr, int n)
{
    int to_do, space_left, i;

    to_do = n;
    space_left = max_long - ctx->cmd_index;

    while (to_do > space_left)
    {
        for (i = 0; i < space_left; ++i)
        {
            ctx->cmd_data[ctx->cmd_index++] = *cptr++;
        }

        cgmb_flush_cmd(ctx, int_flush);
        to_do -= space_left;
        space_left = max_long;
    }

    for (i = 0; i < to_do; ++i)
    {
        ctx->cmd_data[ctx->cmd_index++] = *cptr++;
    }
}

/* Write a CGM binary string */
static void cgmb_string(cgm_context *ctx, const char *cptr, int slen)
{
    int to_do;
    unsigned char byte1, byte2;

    /* put out null strings, however */

    if (slen == 0)
    {
        cgmb_out_bc(ctx, 0);
        return;
    }

    /* now non-trivial stuff */

    if (slen < 255)
    {
        /* simple case */

        cgmb_out_bc(ctx, slen);
        cgmb_out_bs(ctx, cptr, slen);
    }
    else
    {
        cgmb_out_bc(ctx, 255);
        to_do = slen;

        while (to_do > 0)
        {
            if (to_do < max_long)
            {
                /* last one */

                byte1 = to_do >> 8;
                byte2 = to_do & 255;

                cgmb_out_bc(ctx, byte1);
                cgmb_out_bc(ctx, byte2);
                cgmb_out_bs(ctx, cptr, to_do);

                to_do = 0;
            }
            else
            {
                byte1 = (max_long >> 8) | (1 << 7);
                byte2 = max_long & 255;

                cgmb_out_bc(ctx, byte1);
                cgmb_out_bc(ctx, byte2);
                cgmb_out_bs(ctx, cptr, max_long);

                to_do -= max_long;
            }
        }
    }
}

/* Write a signed integer variable */

static void cgmb_gint(cgm_context *ctx, int xin, int precision)
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

    cgmb_out_bs(ctx, buffer, no_out);
}

/* Write an unsigned integer variable */

static void cgmb_uint(cgm_context *ctx, unsigned int xin, int precision)
{
    int i, no_out;
    unsigned char buffer[4];

    no_out = precision / byte_size;

    for (i = no_out - 1; i >= 0; --i)
    {
        buffer[i] = xin & byte_mask;
        xin >>= byte_size;
    }

    cgmb_out_bs(ctx, (char *) buffer, no_out);
}

/* Write fixed point variable */

static void cgmb_fixed(cgm_context *ctx, double xin)
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

    cgmb_gint(ctx, exp_part, real_prec_exp);
    cgmb_uint(ctx, fract_part, real_prec_fract);
}

/* Write IEEE floating point variable */

static void cgmb_float(cgm_context *ctx, double xin)
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
        cgmb_out_bs(ctx, (char *) arry, 4);
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
static void cgmb_dcint(cgm_context *ctx, int xin)
{
    cgmb_uint(ctx, xin, cprec);
}

/* Write a signed int at VDC integer precision */

static void cgmb_vint(cgm_context *ctx, int xin)
{
    cgmb_gint(ctx, xin, 16);
}

/* Write a standard CGM signed int */

static void cgmb_sint(cgm_context *ctx, int xin)
{
    cgmb_gint(ctx, xin, 16);
}

/* Write a signed int at index precision */
static void cgmb_xint(cgm_context *ctx, int xin)
{
    cgmb_gint(ctx, xin, 16);
}

/* Write an unsigned integer at colour index precision */
static void cgmb_cxint(cgm_context *ctx, int xin)
{
    cgmb_uint(ctx, (unsigned) xin, cxprec);
}

/* Write an integer at fixed (16 bit) precision */
static void cgmb_eint(cgm_context *ctx, int xin)

{
    char byte1;
    unsigned char byte2;

    byte1 = xin / 256;
    byte2 = xin & 255;
    cgmb_out_bc(ctx, byte1);
    cgmb_out_bc(ctx, byte2);
}

/* Begin metafile */
static void cgmb_begin_p(cgm_context *ctx, const char *identifier)
{
    cgmb_start_cmd(ctx, 0, (int) B_Mf);

    if (*identifier)
    {
        cgmb_string(ctx, identifier, strlen(identifier));
    }
    else
    {
        cgmb_string(ctx, NULL, 0);
    }

    cgmb_flush_cmd(ctx, final_flush);

    cgmb_fb(ctx);
}

/* End metafile */
static void cgmb_end_p(cgm_context *ctx)
{
    /* put out the end metafile command */

    cgmb_start_cmd(ctx, 0, (int) E_Mf);
    cgmb_flush_cmd(ctx, final_flush);

    /* flush out the buffer */

    cgmb_fb(ctx);
}

/* Start picture */
static void cgmb_bp_p(cgm_context *ctx, const char *pic_name)
{
    cgmb_start_cmd(ctx, 0, (int) B_Pic);

    if (*pic_name)
    {
        cgmb_string(ctx, pic_name, strlen(pic_name));
    }
    else
    {
        cgmb_string(ctx, NULL, 0);
    }

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* Start picture body */
static void cgmb_bpage_p(cgm_context *ctx)
{
    cgmb_start_cmd(ctx, 0, (int) B_Pic_Body);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* End picture */

static void cgmb_epage_p(cgm_context *ctx)
{
    cgmb_start_cmd(ctx, 0, (int) E_Pic);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* Metafile version */

static void cgmb_mfversion_p(cgm_context *ctx, int version)
{
    cgmb_start_cmd(ctx, 1, (int) MfVersion);

    cgmb_sint(ctx, version);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* Metafile description */

static void cgmb_mfdescrip_p(cgm_context *ctx, const char *descrip)
{
    cgmb_start_cmd(ctx, 1, MfDescrip);

    cgmb_string(ctx, descrip, strlen(descrip));

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* VDC type */

static void cgmb_vdctype_p(cgm_context *ctx, cgm::VdcType value)
{
    cgmb_start_cmd(ctx, 1, (int) vdcType);

    cgmb_eint(ctx, (int) value);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* Integer precision */
static void cgmb_intprec_p(cgm_context *ctx, int prec)
{
    cgmb_start_cmd(ctx, 1, (int) IntPrec);

    cgmb_sint(ctx, prec);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* Real precision */
static void cgmb_realprec_p(cgm_context *ctx, int prec, int expWidth, int mantWidth)
{
    cgmb_start_cmd(ctx, 1, (int) RealPrec);

    cgmb_sint(ctx, prec);
    cgmb_sint(ctx, expWidth);
    cgmb_sint(ctx, mantWidth);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* Index precision */
static void cgmb_indexprec_p(cgm_context *ctx, int value)
{
    cgmb_start_cmd(ctx, 1, (int) IndexPrec);

    cgmb_sint(ctx, value);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* Colour precision */

static void cgmb_colprec_p(cgm_context *ctx, int value)
{
    cgmb_start_cmd(ctx, 1, (int) ColPrec);

    cgmb_sint(ctx, value);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* Colour index precision */

static void cgmb_cindprec_p(cgm_context *ctx, int value)
{
    cgmb_start_cmd(ctx, 1, (int) CIndPrec);

    cgmb_sint(ctx, value);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* Colour value extent */

static void cgmb_cvextent_p(cgm_context *ctx, int minRed, int maxRed, int minGreen, int maxGreen, int minBlue, int maxBlue)
{
    cgmb_start_cmd(ctx, 1, (int) CVExtent);

    cgmb_dcint(ctx, minRed);
    cgmb_dcint(ctx, minGreen);
    cgmb_dcint(ctx, minBlue);
    cgmb_dcint(ctx, maxRed);
    cgmb_dcint(ctx, maxGreen);
    cgmb_dcint(ctx, maxBlue);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* Maximum colour index */

static void cgmb_maxcind_p(cgm_context *ctx, int value)
{
    cgmb_start_cmd(ctx, 1, (int) MaxCInd);

    cgmb_cxint(ctx, value);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* Metafile element list */

static void cgmb_mfellist_p(cgm_context *ctx)
{
    int i;

    cgmb_start_cmd(ctx, 1, (int) MfElList);
    cgmb_sint(ctx, n_melements);

    for (i = 2; i < 2 * n_melements; ++i)
    {
        cgmb_xint(ctx, element_list[i]);
    }

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* Font List */

static void cgmb_fontlist_p(cgm_context *ctx, int numFonts, const char *fontNames[])
{
    register int i, slen;
    register char *s;

    for (i = 0, slen = 0; i < numFonts; i++)
    {
        slen += (fontNames[i] != nullptr ? strlen(fontNames[i]) : 0) + 1;
    }
    s = (char *) gks_malloc(slen);

    *s = '\0';
    for (i = 0; i < numFonts; i++)
    {
        strcat(s, fontNames[i]);
        if (i < numFonts - 1)
            strcat(s, " ");
    }

    cgmb_start_cmd(ctx, 1, (int) FontList);

    cgmb_string(ctx, s, strlen(s));

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
    free(s);
}

/* Character announcer */

static void cgmb_cannounce_p(cgm_context *ctx, int value)
{
    cgmb_start_cmd(ctx, 1, (int) CharAnnounce);

    cgmb_eint(ctx, value);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* Scaling mode */

static void cgmb_scalmode_p(cgm_context *ctx, int mode, double value)
{
    cgmb_start_cmd(ctx, 2, (int) ScalMode);

    cgmb_eint(ctx, mode);
    cgmb_float(ctx, value);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* Colour selection mode */

static void cgmb_colselmode_p(cgm_context *ctx, int value)
{
    cgmb_start_cmd(ctx, 2, (int) ColSelMode);

    cgmb_eint(ctx, value);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* line width specification mode */

static void cgmb_lwsmode_p(cgm_context *ctx, int value)
{
    cgmb_start_cmd(ctx, 2, (int) LWidSpecMode);

    cgmb_eint(ctx, value);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* marker size specification mode */

static void cgmb_msmode_p(cgm_context *ctx, int value)
{
    cgmb_start_cmd(ctx, 2, (int) MarkSizSpecMode);

    cgmb_eint(ctx, value);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* VDC extent */

static void cgmb_vdcextent_p(cgm_context *ctx, int xmin, int ymin, int xmax, int ymax)
{
    cgmb_start_cmd(ctx, 2, (int) vdcExtent);

    cgmb_vint(ctx, xmin);
    cgmb_vint(ctx, ymin);
    cgmb_vint(ctx, xmax);
    cgmb_vint(ctx, ymax);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* Background colour */

static void cgmb_backcol_p(cgm_context *ctx, int r, int g, int b)
{
    cgmb_start_cmd(ctx, 2, (int) BackCol);

    cgmb_dcint(ctx, r);
    cgmb_dcint(ctx, g);
    cgmb_dcint(ctx, b);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* VDC integer precision */

static void cgmb_vdcintprec_p(cgm_context *ctx, int value)
{
    cgmb_start_cmd(ctx, 3, (int) vdcIntPrec);

    cgmb_sint(ctx, value);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* Clip rectangle */
static void cgmb_cliprect_p(cgm_context *ctx, int llx, int lly, int urx, int ury)
{
    cgmb_start_cmd(ctx, 3, (int) ClipRect);

    cgmb_vint(ctx, llx);
    cgmb_vint(ctx, lly);
    cgmb_vint(ctx, urx);
    cgmb_vint(ctx, ury);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* Clip indicator */

static void cgmb_clipindic_p(cgm_context *ctx, bool clip_ind)
{
    cgmb_start_cmd(ctx, 3, (int) ClipIndic);

    cgmb_eint(ctx, clip_ind ? 1 : 0);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* Polyline */
static void cgmb_pline_pt(cgm_context *ctx, int numPts, const cgm::Point<int> *pts)
{
    int i;

    cgmb_start_cmd(ctx, 4, (int) PolyLine);

    for (i = 0; i < numPts; ++i)
    {
        cgmb_vint(ctx, pts[i].x);
        cgmb_vint(ctx, pts[i].y);
    }

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}
static void cgmb_pline_p(cgm_context *ctx, int no_pairs, int *x1_ptr, int *y1_ptr)
{
    int i;

    cgmb_start_cmd(ctx, 4, (int) PolyLine);

    for (i = 0; i < no_pairs; ++i)
    {
        cgmb_vint(ctx, x1_ptr[i]);
        cgmb_vint(ctx, y1_ptr[i]);
    }

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* Polymarker */
static void cgmb_pmarker_pt(cgm_context *ctx, int no_pairs, const cgm::Point<int> *pts)
{
    int i;

    cgmb_start_cmd(ctx, 4, (int) PolyMarker);

    for (i = 0; i < no_pairs; ++i)
    {
        cgmb_vint(ctx, pts[i].x);
        cgmb_vint(ctx, pts[i].y);
    }

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}
static void cgmb_pmarker_p(cgm_context *ctx, int no_pairs, int *x1_ptr, int *y1_ptr)
{
    int i;

    cgmb_start_cmd(ctx, 4, (int) PolyMarker);

    for (i = 0; i < no_pairs; ++i)
    {
        cgmb_vint(ctx, x1_ptr[i]);
        cgmb_vint(ctx, y1_ptr[i]);
    }

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* Text */

static void cgmb_text_p(cgm_context *ctx, int x, int y, bool final, const char *buffer)
{
    cgmb_start_cmd(ctx, 4, (int) Text);

    cgmb_vint(ctx, x);
    cgmb_vint(ctx, y);

    cgmb_eint(ctx, final);
    cgmb_string(ctx, buffer, strlen(buffer));

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* Polygon */
static void cgmb_pgon_pt(cgm_context *ctx, int no_pairs, const cgm::Point<int> *pts)
{
    int i;

    cgmb_start_cmd(ctx, 4, (int) C_Polygon);

    for (i = 0; i < no_pairs; ++i)
    {
        cgmb_vint(ctx, pts[i].x);
        cgmb_vint(ctx, pts[i].y);
    }

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}
static void cgmb_pgon_p(cgm_context *ctx, int no_pairs, int *x1_ptr, int *y1_ptr)
{
    int i;

    cgmb_start_cmd(ctx, 4, (int) C_Polygon);

    for (i = 0; i < no_pairs; ++i)
    {
        cgmb_vint(ctx, x1_ptr[i]);
        cgmb_vint(ctx, y1_ptr[i]);
    }

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* Cell array */

static void cgmb_carray_p(cgm_context *ctx, int c1x, int c1y, int c2x, int c2y, int c3x,
    int c3y, int colorPrecision, int dx, int dy, int dimx, const int *array)
{
    int ix, iy, c;

    cgmb_start_cmd(ctx, 4, (int) Cell_Array);

    cgmb_vint(ctx, c1x);
    cgmb_vint(ctx, c1y);
    cgmb_vint(ctx, c2x);
    cgmb_vint(ctx, c2y);
    cgmb_vint(ctx, c3x);
    cgmb_vint(ctx, c3y);

    cgmb_sint(ctx, dx);
    cgmb_sint(ctx, dy);
    cgmb_sint(ctx, colorPrecision);
    cgmb_eint(ctx, 1);

    for (iy = 0; iy < dy; iy++)
    {
        for (ix = 0; ix < dx; ix++)
        {
            c = array[dimx * iy + ix];
            c = Color8Bit(c);
            cgmb_out_bc(ctx, c);
        }

        if (odd(dx))
            cgmb_out_bc(ctx, 0);
    }

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* Line type */

static void cgmb_ltype_p(cgm_context *ctx, int line_type)
{
    cgmb_start_cmd(ctx, 5, (int) LType);

    cgmb_xint(ctx, line_type);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* Line width */

static void cgmb_lwidth_p(cgm_context *ctx, double rmul)
{
    cgmb_start_cmd(ctx, 5, (int) LWidth);

    cgmb_fixed(ctx, rmul);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* Line colour */

static void cgmb_lcolor_p(cgm_context *ctx, int index)
{
    cgmb_start_cmd(ctx, 5, (int) LColour);

    cgmb_cxint(ctx, index);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* Marker type */

static void cgmb_mtype_p(cgm_context *ctx, int marker)
{
    cgmb_start_cmd(ctx, 5, (int) MType);

    cgmb_xint(ctx, marker);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* Marker size */

static void cgmb_msize_p(cgm_context *ctx, double rmul)
{
    cgmb_start_cmd(ctx, 5, (int) MSize);

    cgmb_fixed(ctx, rmul);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* Marker colour */

static void cgmb_mcolor_p(cgm_context *ctx, int index)
{
    cgmb_start_cmd(ctx, 5, (int) MColour);

    cgmb_cxint(ctx, index);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* Text font index */

static void cgmb_tfindex_p(cgm_context *ctx, int index)
{
    cgmb_start_cmd(ctx, 5, (int) TFIndex);

    cgmb_xint(ctx, index);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* Text precision */

static void cgmb_tprec_p(cgm_context *ctx, int precision)
{
    cgmb_start_cmd(ctx, 5, (int) TPrec);

    cgmb_eint(ctx, precision);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* Character expansion factor */

static void cgmb_cexpfac_p(cgm_context *ctx, double factor)
{
    cgmb_start_cmd(ctx, 5, (int) CExpFac);

    cgmb_fixed(ctx, factor);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* Character space */

static void cgmb_cspace_p(cgm_context *ctx, double space)
{
    cgmb_start_cmd(ctx, 5, (int) CSpace);

    cgmb_fixed(ctx, space);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* Text colour */

static void cgmb_tcolor_p(cgm_context *ctx, int index)
{
    cgmb_start_cmd(ctx, 5, (int) TColour);

    cgmb_cxint(ctx, index);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* Character height */

static void cgmb_cheight_p(cgm_context *ctx, int height)
{
    cgmb_start_cmd(ctx, 5, (int) CHeight);

    cgmb_vint(ctx, height);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* Character orientation */

static void cgmb_corient_p(cgm_context *ctx, int x_up, int y_up, int x_base, int y_base)
{
    cgmb_start_cmd(ctx, 5, (int) COrient);

    cgmb_vint(ctx, x_up);
    cgmb_vint(ctx, y_up);
    cgmb_vint(ctx, x_base);
    cgmb_vint(ctx, y_base);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* Text path */

static void cgmb_tpath_p(cgm_context *ctx, int new_path)
{
    cgmb_start_cmd(ctx, 5, (int) TPath);

    cgmb_eint(ctx, new_path);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* Text alignment */

static void cgmb_talign_p(cgm_context *ctx, int hor, int ver, double contHoriz, double contVert)
{
    cgmb_start_cmd(ctx, 5, (int) TAlign);

    cgmb_eint(ctx, hor);
    cgmb_eint(ctx, ver);
    cgmb_fixed(ctx, contHoriz);
    cgmb_fixed(ctx, contVert);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* Interior style */

static void cgmb_intstyle_p(cgm_context *ctx, int style)
{
    cgmb_start_cmd(ctx, 5, (int) IntStyle);

    cgmb_eint(ctx, style);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* Fill colour */

static void cgmb_fillcolor_p(cgm_context *ctx, int index)
{
    cgmb_start_cmd(ctx, 5, (int) FillColour);

    cgmb_cxint(ctx, index);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* Hatch index */

static void cgmb_hindex_p(cgm_context *ctx, int new_index)
{
    cgmb_start_cmd(ctx, 5, (int) HatchIndex);

    cgmb_xint(ctx, new_index);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* Pattern index */

static void cgmb_pindex_p(cgm_context *ctx, int new_index)
{
    cgmb_start_cmd(ctx, 5, (int) PatIndex);

    cgmb_xint(ctx, new_index);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

/* Colour table */

static void cgmb_coltab_c(cgm_context *ctx, int startIndex, int numColors, const cgm::Color *colors)
{
    int i, j;

    cgmb_start_cmd(ctx, 5, (int) ColTab);
    cgmb_cxint(ctx, startIndex);

    for (i = startIndex; i < (startIndex + numColors); ++i)
    {
        cgmb_dcint(ctx, (int) (colors[(i - startIndex)].red * (max_colors - 1)));
        cgmb_dcint(ctx, (int) (colors[(i - startIndex)].green * (max_colors - 1)));
        cgmb_dcint(ctx, (int) (colors[(i - startIndex)].blue * (max_colors - 1)));
    }

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

static void setup_clear_text_context(cgm_context *ctx)
{
    ctx->beginMetafile = cgmt_begin_p;
    ctx->endMetafile = cgmt_end_p;
    ctx->beginPicture = cgmt_bp_p;
    ctx->beginPictureBody = cgmt_bpage_p;
    ctx->endPicture = cgmt_epage_p;
    ctx->metafileVersion = cgmt_mfversion_p;
    ctx->metafileDescription = cgmt_mfdescrip_p;
    ctx->vdcType = cgmt_vdctype_p;
    ctx->intPrecisionClearText = cgmt_intprec_p;
    ctx->realPrecisionClearText = cgmt_realprec_p;
    ctx->indexPrecisionClearText = cgmt_indexprec_p;
    ctx->colorPrecisionClearText = cgmt_colprec_p;
    ctx->colorIndexPrecisionClearText = cgmt_cindprec_p;
    ctx->maximumColorIndex = cgmt_maxcind_p;
    ctx->colorValueExtent = cgmt_cvextent_p;
    ctx->metafileElementList = cgmt_mfellist_p;
    ctx->fontList = cgmt_fontlist_p;
    ctx->characterCodingAnnouncer = cgmt_cannounce_p;
    ctx->scalingMode = cgmt_scalmode_p;
    ctx->colorSelectionMode = cgmt_colselmode_p;
    ctx->lineWidthSpecificationMode = cgmt_lwsmode_p;
    ctx->markerSizeSpecificationMode = cgmt_msmode_p;
    ctx->vdcExtentInt = cgmt_vdcextent_p;
    ctx->backgroundColor = cgmt_backcol_p;
    ctx->vdcIntegerPrecisionClearText = cgmt_vdcintprec_p;
    ctx->clipRectangle = cgmt_cliprect_p;
    ctx->clipIndicator = cgmt_clipindic_p;
    ctx->polylineIntPt = cgmt_pline_pt;
    ctx->polyline = cgmt_pline_p;
    ctx->polymarkerIntPt = cgmt_pmarker_pt;
    ctx->polymarker = cgmt_pmarker_p;
    ctx->textInt = cgmt_text_p;
    ctx->polygonIntPt = cgmt_pgon_pt;
    ctx->polygon = cgmt_pgon_p;
    ctx->cellArray = cgmt_carray_p;
    ctx->lineType = cgmt_ltype_p;
    ctx->lineWidth = cgmt_lwidth_p;
    ctx->lineColor = cgmt_lcolor_p;
    ctx->markerType = cgmt_mtype_p;
    ctx->markerSize = cgmt_msize_p;
    ctx->markerColor = cgmt_mcolor_p;
    ctx->textFontIndex = cgmt_tfindex_p;
    ctx->textPrecision = cgmt_tprec_p;
    ctx->charExpansion = cgmt_cexpfac_p;
    ctx->charSpacing = cgmt_cspace_p;
    ctx->textColor = cgmt_tcolor_p;
    ctx->charHeight = cgmt_cheight_p;
    ctx->charOrientation = cgmt_corient_p;
    ctx->textPath = cgmt_tpath_p;
    ctx->textAlignment = cgmt_talign_p;
    ctx->interiorStyle = cgmt_intstyle_p;
    ctx->fillColor = cgmt_fillcolor_p;
    ctx->hatchIndex = cgmt_hindex_p;
    ctx->patternIndex = cgmt_pindex_p;
    ctx->colorTable = cgmt_coltab_c;

    ctx->buffer_ind = 0;
    ctx->buffer[0] = '\0';
}

static void setup_binary_context(cgm_context *ctx)
{
    ctx->beginMetafile = cgmb_begin_p;
    ctx->endMetafile = cgmb_end_p;
    ctx->beginPicture = cgmb_bp_p;
    ctx->beginPictureBody = cgmb_bpage_p;
    ctx->endPicture = cgmb_epage_p;
    ctx->metafileVersion = cgmb_mfversion_p;
    ctx->metafileDescription = cgmb_mfdescrip_p;
    ctx->vdcType = cgmb_vdctype_p;
    ctx->intPrecisionBinary = cgmb_intprec_p;
    ctx->realPrecisionBinary = cgmb_realprec_p;
    ctx->indexPrecisionBinary = cgmb_indexprec_p;
    ctx->colorPrecisionBinary = cgmb_colprec_p;
    ctx->colorIndexPrecisionBinary = cgmb_cindprec_p;
    ctx->maximumColorIndex = cgmb_maxcind_p;
    ctx->colorValueExtent = cgmb_cvextent_p;
    ctx->metafileElementList = cgmb_mfellist_p;
    ctx->fontList = cgmb_fontlist_p;
    ctx->characterCodingAnnouncer = cgmb_cannounce_p;
    ctx->scalingMode = cgmb_scalmode_p;
    ctx->colorSelectionMode = cgmb_colselmode_p;
    ctx->lineWidthSpecificationMode = cgmb_lwsmode_p;
    ctx->markerSizeSpecificationMode = cgmb_msmode_p;
    ctx->vdcExtentInt = cgmb_vdcextent_p;
    ctx->backgroundColor = cgmb_backcol_p;
    ctx->vdcIntegerPrecisionBinary = cgmb_vdcintprec_p;
    ctx->clipRectangle = cgmb_cliprect_p;
    ctx->clipIndicator = cgmb_clipindic_p;
    ctx->polylineIntPt = cgmb_pline_pt;
    ctx->polyline = cgmb_pline_p;
    ctx->polymarkerIntPt = cgmb_pmarker_pt;
    ctx->polymarker = cgmb_pmarker_p;
    ctx->textInt = cgmb_text_p;
    ctx->polygonIntPt = cgmb_pgon_pt;
    ctx->polygon = cgmb_pgon_p;
    ctx->cellArray = cgmb_carray_p;
    ctx->lineType = cgmb_ltype_p;
    ctx->lineWidth = cgmb_lwidth_p;
    ctx->lineColor = cgmb_lcolor_p;
    ctx->markerType = cgmb_mtype_p;
    ctx->markerSize = cgmb_msize_p;
    ctx->markerColor = cgmb_mcolor_p;
    ctx->textFontIndex = cgmb_tfindex_p;
    ctx->textPrecision = cgmb_tprec_p;
    ctx->charExpansion = cgmb_cexpfac_p;
    ctx->charSpacing = cgmb_cspace_p;
    ctx->textColor = cgmb_tcolor_p;
    ctx->charHeight = cgmb_cheight_p;
    ctx->charOrientation = cgmb_corient_p;
    ctx->textPath = cgmb_tpath_p;
    ctx->textAlignment = cgmb_talign_p;
    ctx->interiorStyle = cgmb_intstyle_p;
    ctx->fillColor = cgmb_fillcolor_p;
    ctx->hatchIndex = cgmb_hindex_p;
    ctx->patternIndex = cgmb_pindex_p;
    ctx->colorTable = cgmb_coltab_c;

    ctx->buffer_ind = 0;
    ctx->buffer[0] = '\0';

    ctx->bfr_index = 0;
}

namespace cgm
{
namespace
{
class BinaryMetafileWriter : public MetafileStreamWriter
{
public:
    BinaryMetafileWriter(std::ostream &stream)
        : MetafileStreamWriter(stream)
    {
        m_context.encode = cgm_binary;
        setup_binary_context(&m_context);
    }

    void beginMetafile(const char *identifier) override;
    void endMetafile() override;
    void beginPicture(char const *identifier) override;
    void beginPictureBody() override;
    void endPicture() override;
    void metafileVersion(int value) override;
    void metafileDescription(char const *value) override;
    void vdcType(VdcType type) override;
    void intPrecisionClearText(int min, int max) override;
    void intPrecisionBinary(int value) override;
    void realPrecisionClearText(float minReal, float maxReal, int digits) override;
    void realPrecisionBinary(RealPrecision prec, int expWidth, int mantWidth) override;
    void indexPrecisionClearText(int min, int max) override;
    void indexPrecisionBinary(int value) override;
    void colorPrecisionClearText(int max) override;
    void colorPrecisionBinary(int value) override;
    void colorIndexPrecisionClearText(int max) override;
    void colorIndexPrecisionBinary(int value) override;
    void maximumColorIndex(int max) override;
    void colorValueExtent(int redMin, int redMax, int greenMin, int greenMax, int blueMin, int blueMax) override;
    void metafileElementList() override;
    void fontList(std::vector<std::string> const &fonts) override;
    void characterCodingAnnouncer(CharCodeAnnouncer value) override;
    void scaleMode(ScaleMode mode, float value) override;
    void colorSelectionMode(ColorMode mode) override;
    void lineWidthSpecificationMode(SpecificationMode mode) override;
    void markerSizeSpecificationMode(SpecificationMode mode) override;
    void vdcExtent(int llx, int lly, int urx, int ury) override;
    void backgroundColor(int red, int green, int blue) override;
    void vdcIntegerPrecisionClearText(int min, int max) override;
    void vdcIntegerPrecisionBinary(int value) override;
    void clipRectangle(int llx, int lly, int urx, int ury) override;
    void clipIndicator(bool enabled) override;
    void polyline(const std::vector<Point<int>> &points) override;
    void polymarker(const std::vector<Point<int>> &points) override;
    void text(Point<int> point, TextFlag flag, const char *text) override;
    void polygon(const std::vector<Point<int>> &points) override;
    void cellArray(Point<int> c1, Point<int> c2, Point<int> c3, int colorPrecision, int nx, int ny, int *colors) override;
    void lineType(int value) override;
    void lineWidth(float value) override;
    void lineColor(int value) override;
    void markerType(int value) override;
    void markerSize(float value) override;
    void markerColor(int value) override;
    void textFontIndex(int value) override;
    void textPrecision(TextPrecision value) override;
    void charExpansion(float value) override;
    void charSpacing(float value) override;
    void textColor(int index) override;
    void charHeight(int value) override;
    void charOrientation(int upX, int upY, int baseX, int baseY) override;
    void textPath(TextPath value) override;
    void textAlignment(HorizAlign horiz, VertAlign vert, float contHoriz, float contVert) override;
    void interiorStyle(InteriorStyle value) override;
    void fillColor(int value) override;
    void hatchIndex(int value) override;
    void patternIndex(int value) override;
    void colorTable(int startIndex, std::vector<Color> const &colors) override;
};

void BinaryMetafileWriter::beginMetafile(const char *identifier)
{
    cgmb_begin_p(&m_context, identifier);
}

void BinaryMetafileWriter::endMetafile()
{
    cgmb_end_p(&m_context);
}

void BinaryMetafileWriter::beginPicture(char const *identifier)
{
    cgmb_bp_p(&m_context, identifier);
}

void BinaryMetafileWriter::beginPictureBody()
{
    cgmb_bpage_p(&m_context);
}

void BinaryMetafileWriter::endPicture()
{
    cgmb_epage_p(&m_context);
}

void BinaryMetafileWriter::metafileVersion(int value)
{
    cgmb_mfversion_p(&m_context, value);
}

void BinaryMetafileWriter::metafileDescription(char const *value)
{
    cgmb_mfdescrip_p(&m_context, value);
}

void BinaryMetafileWriter::vdcType(VdcType type)
{
    cgmb_vdctype_p(&m_context, type);
}

void BinaryMetafileWriter::intPrecisionClearText(int min, int max)
{
    throw std::runtime_error("Unsupported method");
}

void BinaryMetafileWriter::intPrecisionBinary(int value)
{
    cgmb_intprec_p(&m_context, value);
}

void BinaryMetafileWriter::realPrecisionClearText(float minReal, float maxReal, int digits)
{
    throw std::runtime_error("Unsupported method");
}

void BinaryMetafileWriter::realPrecisionBinary(RealPrecision prec, int expWidth, int mantWidth)
{
    cgmb_realprec_p(&m_context, static_cast<int>(prec), expWidth, mantWidth);
}

void BinaryMetafileWriter::indexPrecisionClearText(int min, int max)
{
    throw std::runtime_error("Unsupported method");
}

void BinaryMetafileWriter::indexPrecisionBinary(int value)
{
    cgmb_indexprec_p(&m_context, value);
}

void BinaryMetafileWriter::colorPrecisionClearText(int max)
{
    throw std::runtime_error("Unsupported method");
}

void BinaryMetafileWriter::colorPrecisionBinary(int value)
{
    cgmb_colprec_p(&m_context, value);
}

void BinaryMetafileWriter::colorIndexPrecisionClearText(int max)
{
    throw std::runtime_error("Unsupported method");
}

void BinaryMetafileWriter::colorIndexPrecisionBinary(int value)
{
    cgmb_cindprec_p(&m_context, value);
}

void BinaryMetafileWriter::maximumColorIndex(int max)
{
    cgmb_maxcind_p(&m_context, max);
}

void BinaryMetafileWriter::colorValueExtent(int redMin, int redMax, int greenMin, int greenMax, int blueMin, int blueMax)
{
    cgmb_cvextent_p(&m_context, redMin, redMax, greenMin, greenMax, blueMin, blueMax);
}

void BinaryMetafileWriter::metafileElementList()
{
    cgmb_mfellist_p(&m_context);
}

void BinaryMetafileWriter::fontList(std::vector<std::string> const &fonts)
{
    std::vector<const char *> fontNames;
    fontNames.reserve(fonts.size());
    for (const std::string &font : fonts)
    {
        fontNames.push_back(font.c_str());
    }
    cgmb_fontlist_p(&m_context, static_cast<int>(fonts.size()), fontNames.data());
}

void BinaryMetafileWriter::characterCodingAnnouncer(CharCodeAnnouncer value)
{
    cgmb_cannounce_p(&m_context, static_cast<int>(value));
}

void BinaryMetafileWriter::scaleMode(ScaleMode mode, float value)
{
    cgmb_scalmode_p(&m_context, static_cast<int>(mode), static_cast<double>(value));
}

void BinaryMetafileWriter::colorSelectionMode(ColorMode mode)
{
    cgmb_colselmode_p(&m_context, static_cast<int>(mode));
}

void BinaryMetafileWriter::lineWidthSpecificationMode(SpecificationMode mode)
{
    cgmb_lwsmode_p(&m_context, static_cast<int>(mode));
}

void BinaryMetafileWriter::markerSizeSpecificationMode( SpecificationMode mode )
{
    cgmb_msmode_p(&m_context, static_cast<int>(mode));
}

void BinaryMetafileWriter::vdcExtent(int llx, int lly, int urx, int ury)
{
    cgmb_vdcextent_p(&m_context, llx, lly, urx, ury);
}

void BinaryMetafileWriter::backgroundColor(int red, int green, int blue)
{
    cgmb_backcol_p(&m_context, red, green, blue);
}

void BinaryMetafileWriter::vdcIntegerPrecisionClearText(int min, int max)
{
    throw std::runtime_error("Unsupported");
}

void BinaryMetafileWriter::vdcIntegerPrecisionBinary(int value)
{
    cgmb_vdcintprec_p(&m_context, value);
}

void BinaryMetafileWriter::clipRectangle(int llx, int lly, int urx, int ury)
{
    cgmb_cliprect_p(&m_context, llx, lly, urx, ury);
}

void BinaryMetafileWriter::clipIndicator(bool enabled)
{
    cgmb_clipindic_p(&m_context, enabled);
}

void BinaryMetafileWriter::polyline(const std::vector<Point<int>> &points)
{
    cgmb_pline_pt(&m_context, static_cast<int>(points.size()), points.data());
}

void BinaryMetafileWriter::polymarker(const std::vector<Point<int>> &points)
{
    cgmb_pmarker_pt(&m_context, static_cast<int>(points.size()), points.data());
}

void BinaryMetafileWriter::text(Point<int> point, TextFlag flag, const char *text)
{
    cgmb_text_p(&m_context, point.x, point.y, static_cast<int>(flag), text);
}

void BinaryMetafileWriter::polygon(const std::vector<Point<int>> &points)
{
    cgmb_pgon_pt(&m_context, static_cast<int>(points.size()), points.data());
}

void BinaryMetafileWriter::cellArray(Point<int> c1, Point<int> c2, Point<int> c3, int colorPrecision, int nx, int ny, int *colors)
{
    cgmb_carray_p(&m_context, c1.x, c1.y, c2.x, c2.y, c3.x, c3.y, colorPrecision, nx, ny, nx, colors);
}

void BinaryMetafileWriter::lineType(int value)
{
    cgmb_ltype_p(&m_context, value);
}

void BinaryMetafileWriter::lineWidth(float value)
{
    cgmb_lwidth_p(&m_context, static_cast<double>(value));
}

void BinaryMetafileWriter::lineColor(int value)
{
    cgmb_lcolor_p(&m_context, value);
}

void BinaryMetafileWriter::markerType(int value)
{
    cgmb_mtype_p(&m_context, value);
}

void BinaryMetafileWriter::markerSize(float value)
{
    cgmb_msize_p(&m_context, static_cast<double>(value));
}

void BinaryMetafileWriter::markerColor(int value)
{
    cgmb_mcolor_p(&m_context, value);
}

void BinaryMetafileWriter::textFontIndex(int value)
{
    cgmb_tfindex_p(&m_context, value);
}

void BinaryMetafileWriter::textPrecision(TextPrecision value)
{
    cgmb_tprec_p(&m_context, static_cast<int>(value));
}

void BinaryMetafileWriter::charExpansion(float value)
{
    cgmb_cexpfac_p(&m_context, static_cast<double>(value));
}

void BinaryMetafileWriter::charSpacing(float value)
{
    cgmb_cspace_p(&m_context, static_cast<double>(value));
}

void BinaryMetafileWriter::textColor(int index)
{
    cgmb_tcolor_p(&m_context, index);
}

void BinaryMetafileWriter::charHeight(int value)
{
    cgmb_cheight_p(&m_context, value);
}

void BinaryMetafileWriter::charOrientation(int upX, int upY, int baseX, int baseY)
{
    cgmb_corient_p(&m_context, upX, upY, baseX, baseY);
}

void BinaryMetafileWriter::textPath(TextPath value)
{
    cgmb_tpath_p(&m_context, static_cast<int>(value));
}

void BinaryMetafileWriter::textAlignment(HorizAlign horiz, VertAlign vert, float contHoriz, float contVert)
{
    cgmb_talign_p(&m_context, static_cast<int>(horiz), static_cast<int>(vert), static_cast<double>(contHoriz), static_cast<double>(contVert));
}

void BinaryMetafileWriter::interiorStyle(InteriorStyle value)
{
    cgmb_intstyle_p(&m_context, static_cast<int>(value));
}

void BinaryMetafileWriter::fillColor(int value)
{
    cgmb_fillcolor_p(&m_context, value);
}

void BinaryMetafileWriter::hatchIndex(int value)
{
    cgmb_hindex_p(&m_context, value);
}

void BinaryMetafileWriter::patternIndex(int value)
{
    cgmb_pindex_p(&m_context, value);
}

void BinaryMetafileWriter::colorTable(int startIndex, std::vector<Color> const &colors)
{
    cgmb_coltab_c(&m_context, startIndex, static_cast<int>(colors.size()), colors.data());
}

class ClearTextMetafileWriter : public MetafileStreamWriter
{
public:
    ClearTextMetafileWriter(std::ostream &stream)
        : MetafileStreamWriter(stream)
    {
        m_context.encode = cgm_clear_text;
        setup_clear_text_context(&m_context);
    }

    void beginMetafile(const char *identifier) override;
    void endMetafile() override;
    void beginPicture(char const *identifier) override;
    void beginPictureBody() override;
    void endPicture() override;
    void metafileVersion(int value) override;
    void metafileDescription(char const *value) override;
    void vdcType(VdcType type) override;
    void intPrecisionClearText(int min, int max) override;
    void intPrecisionBinary(int value) override;
    void realPrecisionClearText(float minReal, float maxReal, int digits) override;
    void realPrecisionBinary(RealPrecision prec, int expWidth, int mantWidth) override;
    void indexPrecisionClearText(int min, int max) override;
    void indexPrecisionBinary(int value) override;
    void colorPrecisionClearText(int max) override;
    void colorPrecisionBinary(int value) override;
    void colorIndexPrecisionClearText(int max) override;
    void colorIndexPrecisionBinary(int value) override;
    void maximumColorIndex(int max) override;
    void colorValueExtent(int redMin, int redMax, int greenMin, int greenMax, int blueMin, int blueMax) override;
    void metafileElementList() override;
    void fontList(std::vector<std::string> const &fonts) override;
    void characterCodingAnnouncer(CharCodeAnnouncer value) override;
    void scaleMode(ScaleMode mode, float value) override;
    void colorSelectionMode(ColorMode mode) override;
    void lineWidthSpecificationMode(SpecificationMode mode) override;
    void markerSizeSpecificationMode(SpecificationMode mode) override;
    void vdcExtent(int llx, int lly, int urx, int ury) override;
    void backgroundColor(int red, int green, int blue) override;
    void vdcIntegerPrecisionClearText(int min, int max) override;
    void vdcIntegerPrecisionBinary(int value) override;
    void clipRectangle(int llx, int lly, int urx, int ury) override;
    void clipIndicator(bool enabled) override;
    void polyline(const std::vector<Point<int>> &points) override;
    void polymarker(const std::vector<Point<int>> &points) override;
    void text(Point<int> point, TextFlag flag, const char *text) override;
    void polygon(const std::vector<Point<int>> &points) override;
    void cellArray(Point<int> c1, Point<int> c2, Point<int> c3, int colorPrecision, int nx, int ny, int *colors) override;
    void lineType(int value) override;
    void lineWidth(float value) override;
    void lineColor(int value) override;
    void markerType(int value) override;
    void markerSize(float value) override;
    void markerColor(int value) override;
    void textFontIndex(int value) override;
    void textPrecision(TextPrecision value) override;
    void charExpansion(float value) override;
    void charSpacing(float value) override;
    void textColor(int index) override;
    void charHeight(int value) override;
    void charOrientation(int upX, int upY, int baseX, int baseY) override;
    void textPath(TextPath value) override;
    void textAlignment(HorizAlign horiz, VertAlign vert, float contHoriz, float contVert) override;
    void interiorStyle(InteriorStyle value) override;
    void fillColor(int value) override;
    void hatchIndex(int value) override;
    void patternIndex(int value) override;
    void colorTable(int startIndex, std::vector<Color> const &colors) override;
};

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

void ClearTextMetafileWriter::scaleMode(ScaleMode mode, float value)
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

void ClearTextMetafileWriter::cellArray(Point<int> c1, Point<int> c2, Point<int> c3, int colorPrecision, int nx, int ny, int *colors)
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
    cgmt_tcolor_p(&m_context, index);
}

void ClearTextMetafileWriter::charHeight(int value)
{
    cgmt_cheight_p(&m_context, value);
}

void ClearTextMetafileWriter::charOrientation(int upX, int upY, int baseX, int baseY)
{
    cgmt_corient_p(&m_context, upX, upY, baseX, baseY);
}

void ClearTextMetafileWriter::textPath(TextPath value)
{
    cgmt_tpath_p(&m_context, static_cast<int>(value));
}

void ClearTextMetafileWriter::textAlignment(HorizAlign horiz, VertAlign vert, float contHoriz, float contVert)
{
    cgmt_talign_p(&m_context, static_cast<int>(horiz), static_cast<int>(vert), static_cast<double>(contHoriz), static_cast<double>(contVert));
}

void ClearTextMetafileWriter::interiorStyle(InteriorStyle value)
{
    cgmt_intstyle_p(&m_context, static_cast<int>(value));
}

void ClearTextMetafileWriter::fillColor(int value)
{
    cgmt_fillcolor_p(&m_context, value);
}

void ClearTextMetafileWriter::hatchIndex(int value)
{
    cgmt_hindex_p(&m_context, value);
}

void ClearTextMetafileWriter::patternIndex(int value)
{
    cgmt_pindex_p(&m_context, value);
}

void ClearTextMetafileWriter::colorTable(int startIndex, std::vector<Color> const &colors)
{
    cgmt_coltab_c(&m_context, startIndex, static_cast<int>(colors.size()), colors.data());
}
}        // namespace

std::unique_ptr<MetafileWriter> create(std::ostream &stream, Encoding enc)
{
    if (enc == Encoding::Binary)
    {
        return std::make_unique<BinaryMetafileWriter>(stream);
    }
    if (enc == Encoding::ClearText)
    {
        return std::make_unique<ClearTextMetafileWriter>(stream);
    }

    throw std::runtime_error("Unsupported encoding");
}

}        // namespace cgm
