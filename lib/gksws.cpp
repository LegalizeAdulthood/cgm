#include <iostream>
#include <memory>
#include <sstream>
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

class WorkstationContext
{
public:
    WorkstationContext()
        : conId{},
        encoding{},
        xform{},
        pline{},
        pmark{},
        text{},
        fill{},
        color_t{},
        active{},
        cgm_ctx{}
    {
    }

    std::unique_ptr<cgm::MetafileWriter> writer;
    std::ostringstream buffer;
    int conId;
    encode_enum encoding;
    norm_xform xform;            /* internal transformation */
    line_attributes pline;       /* current polyline attributes */
    marker_attributes pmark;     /* current marker attributes */
    text_attributes text;        /* current text attributes */
    fill_attributes fill;        /* current fill area attributes */
    double color_t[MAX_COLOR * 3];        /* color table */
    bool active;                          /* indicates active workstation */
    cgm_context cgm_ctx;
};

static WorkstationContext *g_context{};

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
static void WC_to_VDC(WorkstationContext *ctx, double xin, double yin, int *xout, int *yout)
{
    double x, y;

    /* Normalization transformation */

    x = ctx->xform.a * xin + ctx->xform.b;
    y = ctx->xform.c * yin + ctx->xform.d;

    /* Segment transformation */

    gks_seg_xform(&x, &y);

    /* Virtual device transformation */

    *xout = (int) (x * max_coord);
    *yout = (int) (y * max_coord);
}

static void init_color_table(double *colors)
{
    for (int i = 0; i < MAX_COLOR; i++)
    {
        gks_inq_rgb(i, &colors[i * 3], &colors[i * 3 + 1], &colors[i * 3 + 2]);
    }
}

static void setup_colors(WorkstationContext *ctx)
{
    std::vector<cgm::Color> colorTable(MAX_COLOR);

    for (int i = 0; i < MAX_COLOR; ++i)
    {
        colorTable[i].red = ctx->color_t[3*i + 0];
        colorTable[i].green = ctx->color_t[3*i + 1];
        colorTable[i].blue = ctx->color_t[3*i + 2];
    }

    ctx->writer->colorTable(0, colorTable);
}

static void set_xform(WorkstationContext *ctx, bool init)
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
        gks_inq_xform(tnr, &errind, ctx->cgm_ctx.wn, ctx->cgm_ctx.vp);
        gks_inq_clip(&errind, &clip_old, clprt);
    }

    gks_inq_current_xformno(&errind, &tnr);
    gks_inq_xform(tnr, &errind, wn_new, vp_new);
    gks_inq_clip(&errind, &clip_new, clprt);

    for (i = 0; i < 4; i++)
    {
        if (vp_new[i] != ctx->cgm_ctx.vp[i])
        {
            ctx->cgm_ctx.vp[i] = vp_new[i];
            update = true;
        }
        if (wn_new[i] != ctx->cgm_ctx.wn[i])
        {
            ctx->cgm_ctx.wn[i] = wn_new[i];
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

                ctx->writer->clipRectangle(clip_rect[0], clip_rect[1], clip_rect[2], clip_rect[3]);
                ctx->writer->clipIndicator(true);
            }
            else
            {
                ctx->writer->clipIndicator(false);
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

                    ctx->writer->clipRectangle(clip_rect[0], clip_rect[1], clip_rect[2], clip_rect[3]);
                    ctx->writer->clipIndicator(true);
                }
                else
                {
                    ctx->writer->clipIndicator(false);
                }
                clip_old = clip_new;
            }
        }
    }
}

static void ws_polyline(WorkstationContext *ctx, int n_points, int *x, int *y)
{
    std::vector<cgm::Point<int>> points;
    for (int i =0; i < n_points; ++i)
    {
        points.push_back(cgm::Point<int>{x[i], y[i]});
    }

    ctx->writer->polyline(points);
}

static void ws_polymarker(WorkstationContext *ctx, int n_points, int *x, int *y)
{
    std::vector<cgm::Point<int>> points;
    for (int i =0; i < n_points; ++i)
    {
        points.push_back(cgm::Point<int>{x[i], y[i]});
    }

    ctx->writer->polymarker(points);
}

static void ws_polygon(WorkstationContext *ctx, int n_points, int *x, int *y)
{
    std::vector<cgm::Point<int>> points;
    for (int i =0; i < n_points; ++i)
    {
        points.push_back(cgm::Point<int>{x[i], y[i]});
    }

    ctx->writer->polygon(points);
}

static void output_points(void (*output_func)(WorkstationContext *, int, int *, int *),
    WorkstationContext *ctx, int n_points, double *x, double *y)
{
    int i;
    static int x_buffer[max_pbuffer], y_buffer[max_pbuffer];

    set_xform(ctx, false);

    if (n_points > max_pbuffer)
    {
        int* d_x_buffer = (int*)gks_malloc(sizeof(double) * n_points);
        int* d_y_buffer = (int*)gks_malloc(sizeof(double) * n_points);

        for (i = 0; i < n_points; i++)
        {
            WC_to_VDC(ctx, x[i], y[i], &d_x_buffer[i], &d_y_buffer[i]);
        }

        output_func(ctx, n_points, d_x_buffer, d_y_buffer);

        free(d_y_buffer);
        free(d_x_buffer);
    }
    else
    {
        for (i = 0; i < n_points; i++)
        {
            WC_to_VDC(ctx, x[i], y[i], &x_buffer[i], &y_buffer[i]);
        }

        output_func(ctx, n_points, x_buffer, y_buffer);
    }
}

static void setup_polyline_attributes( WorkstationContext* ctx, bool init )
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

        if (g_context->encoding == cgm_grafkit)
        {
            if (newpline.type < 0)
                newpline.type = max_std_linetype - newpline.type;
        }

        if (newpline.type != ctx->pline.type)
        {
            ctx->writer->lineType(newpline.type);
            ctx->pline.type = newpline.type;
        }
        if (newpline.width != ctx->pline.width)
        {
            ctx->writer->lineWidth(newpline.width);
            ctx->pline.width = newpline.width;
        }
        if (newpline.color != ctx->pline.color)
        {
            ctx->writer->lineColor(newpline.color);
            ctx->pline.color = newpline.color;
        }
    }
}

static void setup_polymarker_attributes( WorkstationContext* ctx, bool init )
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

        if (g_context->encoding == cgm_grafkit)
        {
            if (newpmark.type < 0)
                newpmark.type = max_std_markertype - newpmark.type;
            if (newpmark.type > 5)
                newpmark.type = 3;
        }

        if (newpmark.type != ctx->pmark.type)
        {
            ctx->writer->markerType(newpmark.type);
            ctx->pmark.type = newpmark.type;
        }
        if (newpmark.width != ctx->pmark.width)
        {
            ctx->writer->markerSize(newpmark.width);
            ctx->pmark.width = newpmark.width;
        }
        if (newpmark.color != ctx->pmark.color)
        {
            ctx->writer->markerColor(newpmark.color);
            ctx->pmark.color = newpmark.color;
        }
    }
}

static void setup_text_attributes( WorkstationContext* ctx, bool init )
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

        if (g_context->encoding == cgm_grafkit)
        {
            if (newtext.font < 0)
                newtext.font = max_std_textfont - newtext.font;
            newtext.prec = 2;
        }

        if (newtext.font != ctx->text.font)
        {
            ctx->writer->textFontIndex(newtext.font);
            ctx->text.font = newtext.font;
        }
        if (newtext.prec != ctx->text.prec)
        {
            ctx->writer->textPrecision(static_cast<cgm::TextPrecision>(newtext.prec));
            ctx->text.prec = newtext.prec;
        }
        if (newtext.expfac != ctx->text.expfac)
        {
            ctx->writer->charExpansion(newtext.expfac);
            ctx->text.expfac = newtext.expfac;
        }
        if (newtext.spacing != ctx->text.spacing)
        {
            ctx->writer->charSpacing(newtext.spacing);
            ctx->text.spacing = newtext.spacing;
        }
        if (newtext.color != ctx->text.color)
        {
            ctx->writer->textColor(newtext.color);
            ctx->text.color = newtext.color;
        }
        if (newtext.height != ctx->text.height)
        {
            ctx->writer->charHeight((int) (newtext.height * max_coord));
            ctx->text.height = newtext.height;
        }
        if ((newtext.upx != ctx->text.upx) || (newtext.upy != ctx->text.upy))
        {
            ctx->writer->charOrientation(newtext.upx, newtext.upy, newtext.upy, -newtext.upx);
            ctx->text.upx = newtext.upx;
            ctx->text.upy = newtext.upy;
        }
        if (newtext.path != ctx->text.path)
        {
            ctx->writer->textPath(static_cast<cgm::TextPath>(newtext.path));
            ctx->text.path = newtext.path;
        }
        if ((newtext.halign != ctx->text.halign) || (newtext.valign != ctx->text.valign))
        {
            ctx->writer->textAlignment(static_cast<cgm::HorizAlign>(newtext.halign), static_cast<cgm::VertAlign>(newtext.valign), 0.0, 0.0);
            ctx->text.halign = newtext.halign;
            ctx->text.valign = newtext.valign;
        }
    }
}

static void setup_fill_attributes( WorkstationContext* ctx, bool init )
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
            ctx->writer->interiorStyle(static_cast<cgm::InteriorStyle>(newfill.intstyle));
            ctx->fill.intstyle = newfill.intstyle;
        }
        if (newfill.color != ctx->fill.color)
        {
            ctx->writer->fillColor(newfill.color);
            ctx->fill.color = newfill.color;
        }
        if (newfill.pattern_index != ctx->fill.pattern_index)
        {
            ctx->writer->patternIndex(newfill.pattern_index);
            ctx->fill.pattern_index = newfill.pattern_index;
        }
        if (newfill.hatch_index != ctx->fill.hatch_index)
        {
            ctx->writer->hatchIndex(newfill.hatch_index);
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

static void cgm_begin_page(WorkstationContext *ctx)
{
    ctx->writer->beginPicture(local_time());

    if (g_context->encoding != cgm_grafkit)
    {
        if (ctx->cgm_ctx.mm > 0)
        {
            ctx->writer->scalingMode(cgm::ScalingMode::Metric, ctx->cgm_ctx.mm);
        }
        else
        {
            ctx->writer->scalingMode(cgm::ScalingMode::Abstract, 0.0);
        }
    }

    ctx->writer->colorSelectionMode(cgm::ColorMode::Direct);

    if (g_context->encoding != cgm_grafkit)
    {
        ctx->writer->lineWidthSpecificationMode(cgm::SpecificationMode::Scaled);
        ctx->writer->markerSizeSpecificationMode(cgm::SpecificationMode::Scaled);
    }

    ctx->writer->vdcExtent(0, 0, ctx->cgm_ctx.xext, ctx->cgm_ctx.yext);
    ctx->writer->backgroundColor(255, 255, 255);

    ctx->writer->beginPictureBody();
    if (g_context->encoding != cgm_clear_text)
    {
        ctx->writer->vdcIntegerPrecisionBinary(16);
    }
    else
    {
        ctx->writer->vdcIntegerPrecisionClearText(-32768, 32767);
    }

    setup_colors(ctx);
    set_xform(ctx, true);
    setup_polyline_attributes(ctx, true);
    setup_polymarker_attributes(ctx, true);
    setup_text_attributes(ctx, true);
    setup_fill_attributes(ctx, true);

    ctx->cgm_ctx.begin_page = false;
}

void gks_drv_cgm(Function fctid, int dx, int dy, int dimx, int *ia,
    int lr1, double *r1, int lr2, double *r2, int lc, char *chars,
    void **cbContext)
{
    g_context = static_cast<WorkstationContext *>(*cbContext);

    cgm::MetafileWriter *ctx{};

    switch (fctid)
    {
    case Function::OpenWorkstation:
        g_context = new WorkstationContext{};
        *cbContext = g_context;

        g_context->conId = ia[1];
        g_context->encoding = cgm_binary;
        if (ia[2] == 7)
        {
            g_context->writer.reset(new cgm::BinaryMetafileWriter(g_context->conId));
        }
        else if (ia[2] == 8)
        {
            g_context->encoding = cgm_clear_text;
            g_context->writer.reset(new cgm::ClearTextMetafileWriter(g_context->conId));
        }
        else if (ia[2] == (7 | 0x50000))
        {
            g_context->writer.reset(new cgm::BinaryMetafileWriter(g_context->conId));
        }
        else
        {
            gks_perror("invalid bit mask (%x)", ia[2]);
            ia[0] = ia[1] = 0;
            return;
        }

        ctx = g_context->writer.get();
        g_context->writer->beginMetafile("GKS, Copyright @ 2001, Josef Heinen");
        g_context->writer->metafileVersion(1);
        g_context->writer->metafileDescription(g_context->encoding == cgm_clear_text ? "GKS 5 CGM Clear Text" : "GKS 5 CGM Binary");

        if (g_context->encoding != cgm_grafkit)
        {
            g_context->writer->vdcType(cgm::VdcType::Integer);
            if (g_context->encoding == cgm_binary)
            {
                ctx->intPrecisionBinary(16);
                ctx->realPrecisionBinary(cgm::RealPrecision::Fixed, 16, 16);
                ctx->indexPrecisionBinary(16);
                ctx->colorPrecisionBinary(cprec);
                ctx->colorIndexPrecisionBinary(cxprec);
            }
            else
            {
                ctx->intPrecisionClearText(-32768, 32767);
                ctx->realPrecisionClearText(-32768., 32768., 4);
                ctx->indexPrecisionClearText(-32768, 32767);
                ctx->colorPrecisionClearText((1 << cprec) - 1);
                ctx->colorIndexPrecisionClearText((1 << cxprec) - 1);
            }
            ctx->maximumColorIndex(MAX_COLOR - 1);
            ctx->colorValueExtent(0, max_colors - 1, 0, max_colors - 1, 0, max_colors - 1);
        }

        ctx->metafileElementList();
        {
            std::vector<std::string> fontNames;
            for (int i = 0; i < max_std_textfont; ++i)
            {
                fontNames.push_back(fonts[map[i]]);
            }
            ctx->fontList(fontNames);
        }

        if (g_context->encoding != cgm_grafkit)
            ctx->characterCodingAnnouncer(cgm::CharCodeAnnouncer::Extended8Bit);

        init_color_table(g_context->color_t);
        g_context->cgm_ctx.xext = g_context->cgm_ctx.yext = max_coord;
        g_context->cgm_ctx.begin_page = true;
        g_context->active = false;
        break;

    case Function::CloseWorkstation:
        ctx->endPicture();
        ctx->endMetafile();
        delete g_context;
        g_context = nullptr;
        break;

    case Function::ActivateWorkstation:
        g_context->active = true;
        break;

    case Function::DeactivateWorkstation:
        g_context->active = false;
        break;

    case Function::ClearWorkstation:
        if (!g_context->cgm_ctx.begin_page)
        {
            ctx->endPicture();
            g_context->cgm_ctx.begin_page = true;
        }
        break;

    case Function::Polyline:
        if (g_context->active)
        {
            if (g_context->cgm_ctx.begin_page)
                cgm_begin_page(g_context);

            setup_polyline_attributes(g_context, false);
            output_points(ws_polyline, g_context, ia[0], r1, r2);
        }
        break;

    case Function::Polymarker:
        if (g_context->active)
        {
            if (g_context->cgm_ctx.begin_page)
                cgm_begin_page(g_context);

            setup_polymarker_attributes(g_context, false);
            output_points(ws_polymarker, g_context, ia[0], r1, r2);
        }
        break;

    case Function::Text:
        if (g_context->active)
        {
            int x, y;

            if (g_context->cgm_ctx.begin_page)
                cgm_begin_page(g_context);

            set_xform(g_context, false);
            setup_text_attributes(g_context, false);

            WC_to_VDC(g_context, r1[0], r2[0], &x, &y);
            ctx->text(cgm::Point<int>{x, y}, cgm::TextFlag::Final, chars);
        }
        break;

    case Function::FillArea:
        if (g_context->active)
        {
            if (g_context->cgm_ctx.begin_page)
                cgm_begin_page(g_context);

            setup_fill_attributes(g_context, false);
            output_points(ws_polygon, g_context, ia[0], r1, r2);
        }
        break;

    case Function::CellArray:
        if (g_context->active)
        {
            int xmin, xmax, ymin, ymax;

            if (g_context->cgm_ctx.begin_page)
                cgm_begin_page(g_context);

            set_xform(g_context, false);

            WC_to_VDC(g_context, r1[0], r2[0], &xmin, &ymin);
            WC_to_VDC(g_context, r1[1], r2[1], &xmax, &ymax);

            ctx->cellArray(cgm::Point<int>{xmin, ymin}, cgm::Point<int>{xmax, ymax}, cgm::Point<int>{xmax, ymin}, max_colors - 1, dx, dy, ia);
        }
        break;

    case Function::SetColorRep:
        if (g_context->cgm_ctx.begin_page)
        {
            g_context->color_t[ia[1] * 3] = r1[0];
            g_context->color_t[ia[1] * 3 + 1] = r1[1];
            g_context->color_t[ia[1] * 3 + 2] = r1[2];
        }
        break;

    case Function::SetWorkstationWindow:
        if (g_context->cgm_ctx.begin_page)
        {
            g_context->cgm_ctx.xext = (int) (max_coord * (r1[1] - r1[0]));
            g_context->cgm_ctx.yext = (int) (max_coord * (r2[1] - r2[0]));
        }
        break;

    case Function::SetWorkstationViewport:
        if (g_context->cgm_ctx.begin_page)
        {
            if (g_context->cgm_ctx.mm > 0)
                g_context->cgm_ctx.mm = (r1[1] - r1[0]) / max_coord * 1000;
        }
        break;
    }
}
