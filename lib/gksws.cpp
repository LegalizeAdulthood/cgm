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

#include "gksws.h"

#include "binary.h"
#include "clear_text.h"
#include "gkscore.h"
#include "impl.h"

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
            cgm::setup_binary_context(g_p);
        }
        else if (ia[2] == 8)
        {
            g_p->encode = cgm_clear_text;
            g_p->flush_buffer_context = &g_p->conid;
            g_p->flush_buffer = gks_flush_buffer;
            cgm::setup_clear_text_context(g_p);
        }
        else if (ia[2] == (7 | 0x50000))
        {
            g_p->encode = cgm_grafkit;
            g_p->flush_buffer_context = &g_p->conid;
            g_p->flush_buffer = gks_flush_buffer;
            cgm::setup_binary_context(g_p);
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
