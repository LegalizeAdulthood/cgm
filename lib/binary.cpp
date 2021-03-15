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

namespace cgm
{

BinaryMetafileWriter::BinaryMetafileWriter(std::ostream &stream)
    : MetafileStreamWriter(stream)
{
    m_context.encode = cgm_binary;
}

BinaryMetafileWriter::BinaryMetafileWriter(int fd)
    : MetafileStreamWriter(fd)
{
    m_context.encode = cgm_binary;
}

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

void BinaryMetafileWriter::scalingMode(ScalingMode mode, float value)
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
    cgm_context *ctx = &m_context;
    cgmb_start_cmd(ctx, 5, (int) CSpace);

    cgmb_fixed(ctx, static_cast<double>(value));

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

void BinaryMetafileWriter::textColor(int index)
{
    cgm_context *ctx = &m_context;
    cgmb_start_cmd(ctx, 5, (int) TColour);

    cgmb_cxint(ctx, index);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

void BinaryMetafileWriter::charHeight(int value)
{
    cgm_context *ctx = &m_context;
    cgmb_start_cmd(ctx, 5, (int) CHeight);

    cgmb_vint(ctx, value);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

void BinaryMetafileWriter::charOrientation(int upX, int upY, int baseX, int baseY)
{
    cgm_context *ctx = &m_context;
    cgmb_start_cmd(ctx, 5, (int) COrient);

    cgmb_vint(ctx, upX);
    cgmb_vint(ctx, upY);
    cgmb_vint(ctx, baseX);
    cgmb_vint(ctx, baseY);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

void BinaryMetafileWriter::textPath(TextPath value)
{
    cgm_context *ctx = &m_context;
    cgmb_start_cmd(ctx, 5, (int) TPath);

    cgmb_eint(ctx, static_cast<int>(value));

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

void BinaryMetafileWriter::textAlignment(HorizAlign horiz, VertAlign vert, float contHoriz, float contVert)
{
    cgm_context *ctx = &m_context;
    cgmb_start_cmd(ctx, 5, (int) TAlign);

    cgmb_eint(ctx, static_cast<int>(horiz));
    cgmb_eint(ctx, static_cast<int>(vert));
    cgmb_fixed(ctx, static_cast<double>(contHoriz));
    cgmb_fixed(ctx, static_cast<double>(contVert));

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

void BinaryMetafileWriter::interiorStyle(InteriorStyle value)
{
    cgm_context *ctx = &m_context;
    cgmb_start_cmd(ctx, 5, (int) IntStyle);

    cgmb_eint(ctx, static_cast<int>(value));

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

void BinaryMetafileWriter::fillColor(int value)
{
    cgm_context *ctx = &m_context;
    cgmb_start_cmd(ctx, 5, (int) FillColour);

    cgmb_cxint(ctx, value);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

void BinaryMetafileWriter::hatchIndex(int value)
{
    cgm_context *ctx = &m_context;
    cgmb_start_cmd(ctx, 5, (int) HatchIndex);

    cgmb_xint(ctx, value);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

void BinaryMetafileWriter::patternIndex(int value)
{
    cgm_context *ctx = &m_context;
    cgmb_start_cmd(ctx, 5, (int) PatIndex);

    cgmb_xint(ctx, value);

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

void BinaryMetafileWriter::colorTable(int startIndex, std::vector<Color> const &colors)
{
    cgm_context *ctx = &m_context;
    const int numColors = static_cast<int>(colors.size());

    cgmb_start_cmd(ctx, 5, (int) ColTab);
    cgmb_cxint(ctx, startIndex);

    for (int i = startIndex; i < (startIndex + numColors); ++i)
    {
        cgmb_dcint(ctx, (int) (colors[(i - startIndex)].red * (max_colors - 1)));
        cgmb_dcint(ctx, (int) (colors[(i - startIndex)].green * (max_colors - 1)));
        cgmb_dcint(ctx, (int) (colors[(i - startIndex)].blue * (max_colors - 1)));
    }

    cgmb_flush_cmd(ctx, final_flush);
    cgmb_fb(ctx);
}

}        // namespace cgm
