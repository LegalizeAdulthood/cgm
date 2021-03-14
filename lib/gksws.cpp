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
#include "gksws.h"

#define odd(number) ((number) &01)
#define nint(a) ((int) ((a) + 0.5))

struct norm_xform
{
    double a, b, c, d;
};

struct line_attributes
{
    int type;
    double width;
    int color;
};

struct marker_attributes
{
    int type;
    double width;
    int color;
};

struct text_attributes
{
    int font;
    int prec;
    double expfac;
    double spacing;
    int color;
    double height;
    int upx;
    int upy;
    int path;
    int halign;
    int valign;
};

struct fill_attributes
{
    int intstyle;
    int hatch_index;
    int pattern_index;
    int color;
};

struct cgm_context
{
    norm_xform xform;            /* internal transformation */
    line_attributes pline;       /* current polyline attributes */
    marker_attributes pmark;     /* current marker attributes */
    text_attributes text;        /* current text attributes */
    fill_attributes fill;        /* current fill area attributes */
    int buffer_ind;              /* output buffer index */
    char buffer[max_buffer + 2]; /* output buffer */
    void *flush_buffer_context;
    void (*flush_buffer)(cgm_context *p, void *data);
    double color_t[MAX_COLOR * 3];        /* color table */
    int conid;                            /* GKS connection id */
    bool active;                          /* indicates active workstation */
    bool begin_page;                      /* indicates begin page */
    double vp[4];                         /* current GKS viewport */
    double wn[4];                         /* current GKS window */
    int xext, yext;                       /* VDC extent */
    double mm;                            /* metric size in mm */
    char cmd_buffer[hdr_long + max_long]; /* where we buffer output */
    char *cmd_hdr;                        /* the command header */
    char *cmd_data;                       /* the command data */
    int cmd_index;                        /* index into the command data */
    int bfr_index;                        /* index into the buffer */
    int partition;                        /* which partition in the output */
    enum encode_enum encode;              /* type of encoding */

    void (*beginMetafile)(cgm_context *p, const char *identifier);
    void (*endMetafile)(cgm_context *p);
    void (*beginPicture)(cgm_context *p, const char *identifier);
    void (*beginPictureBody)(cgm_context *p);
    void (*endPicture)(cgm_context *p);
    void (*metafileVersion)(cgm_context *p, int value);
    void (*metafileDescription)(cgm_context *p, const char *value);
    void (*vdcType)(cgm_context *p, cgm::VdcType type);
    void (*intPrecisionClearText)(cgm_context *p, int min, int max);
    void (*intPrecisionBinary)(cgm_context *p, int value);
    void (*realPrecisionClearText)(cgm_context *p, double minReal, double maxReal, int digits);
    void (*realPrecisionBinary)(cgm_context *p, int prec, int expWidth, int mantWidth);
    void (*indexPrecisionClearText)(cgm_context *p, int min, int max);
    void (*indexPrecisionBinary)(cgm_context *p, int value);
    void (*colorPrecisionClearText)(cgm_context *p, int max);
    void (*colorPrecisionBinary)(cgm_context *p, int value);
    void (*colorIndexPrecisionClearText)(cgm_context *p, int max);
    void (*colorIndexPrecisionBinary)(cgm_context *p, int value);
    void (*maximumColorIndex)(cgm_context *p, int max);
    void (*colorValueExtent)(cgm_context *p, int redMin, int redMax, int greenMin, int greenMax, int blueMin, int blueMax);
    void (*metafileElementList)(cgm_context *p);
    void (*fontList)(cgm_context *p, int numFonts, const char **fontNames);
    // character set list
    void (*characterCodingAnnouncer)(cgm_context *p, int value);
    void (*scalingMode)(cgm_context *p, int mode, double value);
    void (*colorSelectionMode)(cgm_context *p, int mode);
    void (*lineWidthSpecificationMode)(cgm_context *p, int mode);
    void (*markerSizeSpecificationMode)(cgm_context *p, int mode);
    void (*vdcExtentInt)(cgm_context *p, int llx, int lly, int urx, int ury);
    void (*backgroundColor)(cgm_context *p, int red, int green, int blue);
    void (*vdcIntegerPrecisionClearText)(cgm_context *p, int min, int max);
    void (*vdcIntegerPrecisionBinary)(cgm_context *p, int value);
    void (*clipRectangle)(cgm_context *p, int llx, int lly, int urx, int ury);
    void (*clipIndicator)(cgm_context *p, bool clip_ind);
    void (*polylineIntPt)(cgm_context *p, int numPoints, const cgm::Point<int> *points);
    void (*polyline)(cgm_context *ctx, int no_pairs, int *x1_ptr, int *y1_ptr);
    void (*polymarkerIntPt)(cgm_context *p, int numPoints, const cgm::Point<int> *points);
    void (*polymarker)(cgm_context *p, int no_pairs, int *x1_ptr, int *y1_ptr);
    void (*textInt)(cgm_context *p, int x, int y, bool flag, const char *text);
    void (*polygonIntPt)(cgm_context *p, int numPoints, const cgm::Point<int> *points);
    void (*polygon)(cgm_context *p, int no_pairs, int *x1_ptr, int *y1_ptr);
    void (*cellArray)(cgm_context *p, int c1x, int c1y, int c2x, int c2y, int c3x, int c3y, int colorPrecision, int nx, int ny, int dimx, const int *colors);
    void (*lineType)(cgm_context *p, int line_type);
    void (*lineWidth)(cgm_context *p, double rmul);
    void (*lineColor)(cgm_context *p, int index);
    void (*markerType)(cgm_context *p, int marker_type);
    void (*markerSize)(cgm_context *p, double size);
    void (*markerColor)(cgm_context *p, int index);
    void (*textFontIndex)(cgm_context *p, int index);
    void (*textPrecision)(cgm_context *p, int value);
    void (*charExpansion)(cgm_context *p, double value);
    void (*charSpacing)(cgm_context *p, double value);
    void (*textColor)(cgm_context *p, int index);
    void (*charHeight)(cgm_context *p, int value);
    void (*charOrientation)(cgm_context *p, int upX, int upY, int baseX, int baseY);
    void (*textPath)(cgm_context *p, int value);
    void (*textAlignment)(cgm_context *p, int horiz, int vert, double contHoriz, double contVert);
    void (*interiorStyle)(cgm_context *p, int value);
    void (*fillColor)(cgm_context *p, int value);
    void (*hatchIndex)(cgm_context *p, int value);
    void (*patternIndex)(cgm_context *p, int value);
    void (*colorTable)(cgm_context *p, int startIndex, int numColors, const cgm::Color *colors);
};

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

static void init_color_table(cgm_context *ctx)
{
    int i, j;

    for (i = 0; i < MAX_COLOR; i++)
    {
        j = i;
        gks_inq_rgb(j, &ctx->color_t[i * 3], &ctx->color_t[i * 3 + 1],
            &ctx->color_t[i * 3 + 2]);
    }
}

static void setup_colors(cgm_context *ctx)
{
    cgm::Color colorTable[MAX_COLOR];

    for (int i = 0; i < MAX_COLOR; ++i)
    {
        colorTable[i].red = ctx->color_t[3*i + 0];
        colorTable[i].green = ctx->color_t[3*i + 1];
        colorTable[i].blue = ctx->color_t[3*i + 2];
    }

    ctx->colorTable(ctx, 0, MAX_COLOR, &colorTable[0]);
}

static void set_xform(cgm_context *ctx, bool init)
{
    int errind, tnr;
    double vp_new[4], wn_new[4];
    double clprt[4];
    static int clip_old;
    int clip_new, clip_rect[4];
    int i;
    bool update = false;

    if (init)
    {
        gks_inq_current_xformno(&errind, &tnr);
        gks_inq_xform(tnr, &errind, ctx->wn, ctx->vp);
        gks_inq_clip(&errind, &clip_old, clprt);
    }

    gks_inq_current_xformno(&errind, &tnr);
    gks_inq_xform(tnr, &errind, wn_new, vp_new);
    gks_inq_clip(&errind, &clip_new, clprt);

    for (i = 0; i < 4; i++)
    {
        if (vp_new[i] != ctx->vp[i])
        {
            ctx->vp[i] = vp_new[i];
            update = true;
        }
        if (wn_new[i] != ctx->wn[i])
        {
            ctx->wn[i] = wn_new[i];
            update = true;
        }
    }

    if (init || update || (clip_old != clip_new))
    {
        ctx->xform.a = (vp_new[1] - vp_new[0]) / (wn_new[1] - wn_new[0]);
        ctx->xform.b = vp_new[0] - wn_new[0] * ctx->xform.a;
        ctx->xform.c = (vp_new[3] - vp_new[2]) / (wn_new[3] - wn_new[2]);
        ctx->xform.d = vp_new[2] - wn_new[2] * ctx->xform.c;

        if (init)
        {
            if (clip_new)
            {
                clip_rect[0] = (int) (vp_new[0] * max_coord);
                clip_rect[1] = (int) (vp_new[2] * max_coord);
                clip_rect[2] = (int) (vp_new[1] * max_coord);
                clip_rect[3] = (int) (vp_new[3] * max_coord);

                ctx->clipRectangle(ctx, clip_rect[0], clip_rect[1], clip_rect[2], clip_rect[3]);
                ctx->clipIndicator(ctx, true);
            }
            else
            {
                ctx->clipIndicator(ctx, false);
            }
            clip_old = clip_new;
        }
        else
        {
            if ((clip_old != clip_new) || update)
            {
                if (clip_new)
                {
                    clip_rect[0] = (int) (vp_new[0] * max_coord);
                    clip_rect[1] = (int) (vp_new[2] * max_coord);
                    clip_rect[2] = (int) (vp_new[1] * max_coord);
                    clip_rect[3] = (int) (vp_new[3] * max_coord);

                    ctx->clipRectangle(ctx, clip_rect[0], clip_rect[1], clip_rect[2], clip_rect[3]);
                    ctx->clipIndicator(ctx, true);
                }
                else
                {
                    ctx->clipIndicator(ctx, false);
                }
                clip_old = clip_new;
            }
        }
    }
}

static void output_points(void (*output_func)(cgm_context *, int, int *, int *),
    cgm_context *ctx, int n_points, double *x, double *y)
{
    int i;
    static int x_buffer[max_pbuffer], y_buffer[max_pbuffer];

    set_xform(g_p, false);

    if (n_points > max_pbuffer)
    {
        int* d_x_buffer = (int*)gks_malloc(sizeof(double) * n_points);
        int* d_y_buffer = (int*)gks_malloc(sizeof(double) * n_points);

        for (i = 0; i < n_points; i++)
        {
            WC_to_VDC(x[i], y[i], &d_x_buffer[i], &d_y_buffer[i]);
        }

        output_func(ctx, n_points, d_x_buffer, d_y_buffer);

        free(d_y_buffer);
        free(d_x_buffer);
    }
    else
    {
        for (i = 0; i < n_points; i++)
        {
            WC_to_VDC(x[i], y[i], &x_buffer[i], &y_buffer[i]);
        }

        output_func(ctx, n_points, x_buffer, y_buffer);
    }
}

static void setup_polyline_attributes(cgm_context *ctx, bool init)
{
    line_attributes newpline;
    int errind;

    if (init)
    {
        ctx->pline.type = 1;
        ctx->pline.width = 1.0;
        ctx->pline.color = 1;
    }
    else
    {
        gks_inq_pline_linetype(&errind, &newpline.type);
        gks_inq_pline_linewidth(&errind, &newpline.width);
        gks_inq_pline_color_index(&errind, &newpline.color);

        if (ctx->encode == cgm_grafkit)
        {
            if (newpline.type < 0)
                newpline.type = max_std_linetype - newpline.type;
        }

        if (newpline.type != ctx->pline.type)
        {
            ctx->lineType(ctx, newpline.type);
            ctx->pline.type = newpline.type;
        }
        if (newpline.width != ctx->pline.width)
        {
            ctx->lineWidth(ctx, newpline.width);
            ctx->pline.width = newpline.width;
        }
        if (newpline.color != ctx->pline.color)
        {
            ctx->lineColor(ctx, newpline.color);
            ctx->pline.color = newpline.color;
        }
    }
}

static void setup_polymarker_attributes(cgm_context *ctx, bool init)
{
    marker_attributes newpmark;
    int errind;

    if (init)
    {
        ctx->pmark.type = 3;
        ctx->pmark.width = 1.0;
        ctx->pmark.color = 1;
    }
    else
    {
        gks_inq_pmark_type(&errind, &newpmark.type);
        gks_inq_pmark_size(&errind, &newpmark.width);
        gks_inq_pmark_color_index(&errind, &newpmark.color);

        if (ctx->encode == cgm_grafkit)
        {
            if (newpmark.type < 0)
                newpmark.type = max_std_markertype - newpmark.type;
            if (newpmark.type > 5)
                newpmark.type = 3;
        }

        if (newpmark.type != ctx->pmark.type)
        {
            ctx->markerType(ctx, newpmark.type);
            ctx->pmark.type = newpmark.type;
        }
        if (newpmark.width != ctx->pmark.width)
        {
            ctx->markerSize(ctx, newpmark.width);
            ctx->pmark.width = newpmark.width;
        }
        if (newpmark.color != ctx->pmark.color)
        {
            ctx->markerColor(ctx, newpmark.color);
            ctx->pmark.color = newpmark.color;
        }
    }
}

static void setup_text_attributes(cgm_context *ctx, bool init)
{
    text_attributes newtext;
    int errind;
    double upx, upy, norm;

    if (init)
    {
        ctx->text.font = 1;
        ctx->text.prec = 0;
        ctx->text.expfac = 1.0;
        ctx->text.spacing = 0.0;
        ctx->text.color = 1;
        ctx->text.height = 0.01;
        ctx->text.upx = 0;
        ctx->text.upy = max_coord;
        ctx->text.path = 0;
        ctx->text.halign = 0;
        ctx->text.valign = 0;
    }
    else
    {
        gks_inq_text_fontprec(&errind, &newtext.font, &newtext.prec);
        gks_inq_text_expfac(&errind, &newtext.expfac);
        gks_inq_text_spacing(&errind, &newtext.spacing);
        gks_inq_text_color_index(&errind, &newtext.color);
        gks_set_chr_xform();
        gks_chr_height(&newtext.height);
        gks_inq_text_upvec(&errind, &upx, &upy);
        upx *= ctx->xform.a;
        upy *= ctx->xform.c;
        gks_seg_xform(&upx, &upy);
        norm = fabs(upx) > fabs(upy) ? fabs(upx) : fabs(upy);
        newtext.upx = (int) (upx / norm * max_coord);
        newtext.upy = (int) (upy / norm * max_coord);
        gks_inq_text_path(&errind, &newtext.path);
        gks_inq_text_align(&errind, &newtext.halign, &newtext.valign);

        if (ctx->encode == cgm_grafkit)
        {
            if (newtext.font < 0)
                newtext.font = max_std_textfont - newtext.font;
            newtext.prec = 2;
        }

        if (newtext.font != ctx->text.font)
        {
            ctx->textFontIndex(ctx, newtext.font);
            ctx->text.font = newtext.font;
        }
        if (newtext.prec != ctx->text.prec)
        {
            ctx->textPrecision(ctx, newtext.prec);
            ctx->text.prec = newtext.prec;
        }
        if (newtext.expfac != ctx->text.expfac)
        {
            ctx->charExpansion(ctx, newtext.expfac);
            ctx->text.expfac = newtext.expfac;
        }
        if (newtext.spacing != ctx->text.spacing)
        {
            ctx->charSpacing(ctx, newtext.spacing);
            ctx->text.spacing = newtext.spacing;
        }
        if (newtext.color != ctx->text.color)
        {
            ctx->textColor(ctx, newtext.color);
            ctx->text.color = newtext.color;
        }
        if (newtext.height != ctx->text.height)
        {
            ctx->charHeight(ctx, (int) (newtext.height * max_coord));
            ctx->text.height = newtext.height;
        }
        if ((newtext.upx != ctx->text.upx) || (newtext.upy != ctx->text.upy))
        {
            ctx->charOrientation(ctx, newtext.upx, newtext.upy, newtext.upy, -newtext.upx);
            ctx->text.upx = newtext.upx;
            ctx->text.upy = newtext.upy;
        }
        if (newtext.path != ctx->text.path)
        {
            ctx->textPath(ctx, newtext.path);
            ctx->text.path = newtext.path;
        }
        if ((newtext.halign != ctx->text.halign) || (newtext.valign != ctx->text.valign))
        {
            ctx->textAlignment(ctx, newtext.halign, newtext.valign, 0.0, 0.0);
            ctx->text.halign = newtext.halign;
            ctx->text.valign = newtext.valign;
        }
    }
}

static void setup_fill_attributes(cgm_context *ctx, bool init)
{
    fill_attributes newfill;
    int errind;

    if (init)
    {
        ctx->fill.intstyle = 0;
        ctx->fill.color = 1;
        ctx->fill.pattern_index = 1;
        ctx->fill.hatch_index = 1;
    }
    else
    {
        gks_inq_fill_int_style(&errind, &newfill.intstyle);
        gks_inq_fill_color_index(&errind, &newfill.color);
        gks_inq_fill_style_index(&errind, &newfill.pattern_index);
        gks_inq_fill_style_index(&errind, &newfill.hatch_index);

        if (newfill.intstyle != ctx->fill.intstyle)
        {
            ctx->interiorStyle(ctx, newfill.intstyle);
            ctx->fill.intstyle = newfill.intstyle;
        }
        if (newfill.color != ctx->fill.color)
        {
            ctx->fillColor(ctx, newfill.color);
            ctx->fill.color = newfill.color;
        }
        if (newfill.pattern_index != ctx->fill.pattern_index)
        {
            ctx->patternIndex(ctx, newfill.pattern_index);
            ctx->fill.pattern_index = newfill.pattern_index;
        }
        if (newfill.hatch_index != ctx->fill.hatch_index)
        {
            ctx->hatchIndex(ctx, newfill.hatch_index);
            ctx->fill.hatch_index = newfill.hatch_index;
        }
    }
}

static const char *local_time()
{
    time_t time_val;
    static const char *weekday[7] = {
        "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday",
        "Saturday"
    };
    static const char *month[12] = {
        "January", "February", "March", "April", "May", "June", "July",
        "August", "September", "October", "November", "December"
    };
    static char time_string[81];

    time(&time_val);
    struct tm* time_structure = localtime(&time_val);

    sprintf(time_string, "%s, %s %d, 19%d %d:%02d:%02d",
        weekday[time_structure->tm_wday], month[time_structure->tm_mon],
        time_structure->tm_mday, time_structure->tm_year,
        time_structure->tm_hour, time_structure->tm_min,
        time_structure->tm_sec);

    return time_string;
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

static void cgm_begin_page(cgm_context *ctx)
{
    ctx->beginPicture(ctx, local_time());

    if (ctx->encode != cgm_grafkit)
    {
        if (ctx->mm > 0)
        {
            ctx->scalingMode(ctx, 1, ctx->mm);
        }
        else
        {
            ctx->scalingMode(ctx, 0, 0.0);
        }
    }

    ctx->colorSelectionMode(ctx, 1);

    if (ctx->encode != cgm_grafkit)
    {
        ctx->lineWidthSpecificationMode(ctx, 1);
        ctx->markerSizeSpecificationMode(ctx, 1);
    }

    ctx->vdcExtentInt(ctx, 0, 0, ctx->xext, ctx->yext);
    ctx->backgroundColor(ctx, 255, 255, 255);

    ctx->beginPictureBody(ctx);
    if (ctx->encode != cgm_clear_text)
    {
        ctx->vdcIntegerPrecisionBinary(ctx, 16);
    }
    else
    {
        ctx->vdcIntegerPrecisionClearText(ctx, -32768, 32767);
    }

    setup_colors(g_p);

    set_xform(g_p, true);

    setup_polyline_attributes(g_p, true);
    setup_polymarker_attributes(g_p, true);
    setup_text_attributes(g_p, true);
    setup_fill_attributes(g_p, true);

    ctx->begin_page = false;
}

static void gks_flush_buffer(cgm_context *ctx, void *cbData)
{
    gks_write_file(ctx->conid, ctx->buffer, ctx->buffer_ind);
}

static void gks_drv_cgm(Function fctid, int dx, int dy, int dimx, int *ia,
    int lr1, double *r1, int lr2, double *r2, int lc, char *chars,
    void **context)
{
    g_p = (cgm_context *) *context;

    switch (fctid)
    {
    case Function::OpenWorkstation:
        g_p = (cgm_context *) gks_malloc(sizeof(cgm_context));

        g_p->conid = ia[1];

        if (ia[2] == 7)
        {
            g_p->encode = cgm_binary;
            g_p->flush_buffer_context = &g_p->conid;
            g_p->flush_buffer = gks_flush_buffer;
            setup_binary_context(g_p);
        }
        else if (ia[2] == 8)
        {
            g_p->encode = cgm_clear_text;
            g_p->flush_buffer_context = &g_p->conid;
            g_p->flush_buffer = gks_flush_buffer;
            setup_clear_text_context(g_p);
        }
        else if (ia[2] == (7 | 0x50000))
        {
            g_p->encode = cgm_grafkit;
            g_p->flush_buffer_context = &g_p->conid;
            g_p->flush_buffer = gks_flush_buffer;
            setup_binary_context(g_p);
        }
        else
        {
            gks_perror("invalid bit mask (%x)", ia[2]);
            ia[0] = ia[1] = 0;
            return;
        }

        if (getenv("CGM_SCALE_MODE_METRIC") != nullptr)
            g_p->mm = 0.19685 / max_coord * 1000;
        else
            g_p->mm = 0;

        g_p->beginMetafile(g_p, "GKS, Copyright @ 2001, Josef Heinen");
        g_p->metafileVersion(g_p, 1);
        g_p->metafileDescription(g_p, g_p->encode == cgm_clear_text ? "GKS 5 CGM Clear Text" : "GKS 5 CGM Binary");

        if (g_p->encode != cgm_grafkit)
        {
            g_p->vdcType(g_p, cgm::VdcType::Integer);
            if (g_p->encode == cgm_binary)
            {
                g_p->intPrecisionBinary(g_p, 16);
                g_p->realPrecisionBinary(g_p, 1, 16, 16);
                g_p->indexPrecisionBinary(g_p, 16);
                g_p->colorPrecisionBinary(g_p, cprec);
                g_p->colorIndexPrecisionBinary(g_p, cxprec);
            }
            else
            {
                g_p->intPrecisionClearText(g_p, -32768, 32767);
                g_p->realPrecisionClearText(g_p, -32768., 32768., 4);
                g_p->indexPrecisionClearText(g_p, -32768, 32767);
                g_p->colorPrecisionClearText(g_p, (1 << cprec) - 1);
                g_p->colorIndexPrecisionClearText(g_p, (1 << cxprec) - 1);
            }
            g_p->maximumColorIndex(g_p, MAX_COLOR - 1);
            g_p->colorValueExtent(g_p, 0, max_colors - 1, 0, max_colors - 1, 0, max_colors - 1);
        }

        g_p->metafileElementList(g_p);
        {
            const char *fontNames[max_std_textfont];
            for (int i = 0; i < max_std_textfont; ++i)
            {
                fontNames[i] = fonts[map[i]];
            }
            g_p->fontList(g_p, max_std_textfont, fontNames);
        }

        if (g_p->encode != cgm_grafkit)
            g_p->characterCodingAnnouncer(g_p, 3);

        init_color_table(g_p);

        g_p->xext = g_p->yext = max_coord;

        g_p->begin_page = true;
        g_p->active = false;

        *context = g_p;
        break;

    case Function::CloseWorkstation:
        g_p->endPicture(g_p);
        g_p->endMetafile(g_p);

        free(g_p);
        break;

    case Function::ActivateWorkstation:
        g_p->active = true;
        break;

    case Function::DeactivateWorkstation:
        g_p->active = false;
        break;

    case Function::ClearWorkstation:
        if (!g_p->begin_page)
        {
            g_p->endPicture(g_p);
            g_p->begin_page = true;
        }
        break;

    case Function::Polyline:
        if (g_p->active)
        {
            if (g_p->begin_page)
                cgm_begin_page(g_p);

            setup_polyline_attributes(g_p, false);
            output_points(g_p->polyline, g_p, ia[0], r1, r2);
        }
        break;

    case Function::Polymarker:
        if (g_p->active)
        {
            if (g_p->begin_page)
                cgm_begin_page(g_p);

            setup_polymarker_attributes(g_p, false);
            output_points(g_p->polymarker, g_p, ia[0], r1, r2);
        }
        break;

    case Function::Text:
        if (g_p->active)
        {
            int x, y;

            if (g_p->begin_page)
                cgm_begin_page(g_p);

            set_xform(g_p, false);
            setup_text_attributes(g_p, false);

            WC_to_VDC(r1[0], r2[0], &x, &y);
            g_p->textInt(g_p, x, y, true, chars);
        }
        break;

    case Function::FillArea:
        if (g_p->active)
        {
            if (g_p->begin_page)
                cgm_begin_page(g_p);

            setup_fill_attributes(g_p, false);
            output_points(g_p->polygon, g_p, ia[0], r1, r2);
        }
        break;

    case Function::CellArray:
        if (g_p->active)
        {
            int xmin, xmax, ymin, ymax;

            if (g_p->begin_page)
                cgm_begin_page(g_p);

            set_xform(g_p, false);

            WC_to_VDC(r1[0], r2[0], &xmin, &ymin);
            WC_to_VDC(r1[1], r2[1], &xmax, &ymax);

            g_p->cellArray(g_p, xmin, ymin, xmax, ymax, xmax, ymin, max_colors - 1, dx, dy, dimx, ia);
        }
        break;

    case Function::SetColorRep:
        if (g_p->begin_page)
        {
            g_p->color_t[ia[1] * 3] = r1[0];
            g_p->color_t[ia[1] * 3 + 1] = r1[1];
            g_p->color_t[ia[1] * 3 + 2] = r1[2];
        }
        break;

    case Function::SetWorkstationWindow:
        if (g_p->begin_page)
        {
            g_p->xext = (int) (max_coord * (r1[1] - r1[0]));
            g_p->yext = (int) (max_coord * (r2[1] - r2[0]));
        }
        break;

    case Function::SetWorkstationViewport:
        if (g_p->begin_page)
        {
            if (g_p->mm > 0)
                g_p->mm = (r1[1] - r1[0]) / max_coord * 1000;
        }
        break;
    }
}
