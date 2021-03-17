#pragma once

#include "impl.h"

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
    int buffer_ind;              /* output buffer index */
    char buffer[max_buffer + 2]; /* output buffer */
    void *flush_buffer_context;
    void (*flush_buffer)(cgm_context *p, void *data);
    double mm;                            /* metric size in mm */
    char *cmd_hdr;                        /* the command header */
    char *cmd_data;                       /* the command data */
    int cmd_index;                        /* index into the command data */
    int bfr_index;                        /* index into the buffer */
    int partition;                        /* which partition in the output */
    enum encode_enum encode;              /* type of encoding */
};
