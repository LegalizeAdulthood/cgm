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
