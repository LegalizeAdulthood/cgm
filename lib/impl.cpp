#include "impl.h"

/* metafile elements */
const char *const cgmt_delim[] = {
    "", "BegMF", "EndMF", "BegPic", "BegPicBody", "EndPic"
};

/* metafile descriptors */
const char *const cgmt_mfdesc[] = {
    "", "MFVersion", "MFDesc", "VDCType", "IntegerPrec", "RealPrec",
    "IndexPrec", "ColrPrec", "ColrIndexPrec", "MaxColrIndex", "ColrValueExt",
    "MFElemList", "MFDefaults", "FontList", "CharSetList", "CharCoding"
};

/* page descriptors */
const char *const cgmt_pdesc[] = {
    "", "ScaleMode", "ColrMode", "LineWidthMode", "MarkerSizeMode",
    "EdgeWidthMode", "VDCExt", "BackColr"
};

/* control elements */
const char *const cgmt_control[] = {
    "", "VDCIntegerPrec", "VDCRealPrec", "AuxColr",
    "Transparency", "ClipRect", "Clip"
};

/* graphical primitives */
const char *const cgmt_gprim[] = {
    "", "Line", "DisjtLine", "Marker", "Text", "RestrText", "ApndText",
    "Polygon", "PolygonSet", "CellArray", "GDP", "Rect", "Circle", "Arc3Pt",
    "Arc3PtClose", "ArcCtr", "ArcCtrClose", "Ellipse", "EllipArc",
    "EllipArcClose"
};

/* attribute elements */
const char *const cgmt_attr[] = {
    "", "LineIndex", "LineType", "LineWidth", "LineColr", "MarkerIndex",
    "MarkerType", "MarkerSize", "MarkerColr", "TextIndex", "TextFontIndex",
    "TextPrec", "CharExpan", "CharSpace", "TextColr", "CharHeight", "CharOri",
    "TextPath", "TextAlign", "CharSetIndex", "AltCharSetIndex", "FillIndex",
    "IntStyle", "FillColr", "HatchIndex", "PatIndex", "EdgeIndex", "EdgeType",
    "EdgeWidth", "EdgeColr", "EdgeVis", "FillRefPt", "PatTable", "PatSize",
    "ColrTable", "ASF"
};

/* escape element */
const char *const cgmt_escape[] = {
    "", "Escape"
};

/* external elements */
const char *const cgmt_external[] = {
    "", "Message", "ApplData"
};

const char *const *const cgmt_cptr[8] = {
    cgmt_delim, cgmt_mfdesc, cgmt_pdesc, cgmt_control, cgmt_gprim, cgmt_attr,
    cgmt_escape, cgmt_external
};

/* used metafile elements */
const int element_list[n_melements * 2] = {
    0, (int) No_Op,
    0, (int) B_Mf,
    0, (int) E_Mf,
    0, (int) B_Pic,
    0, (int) B_Pic_Body,
    0, (int) E_Pic,
    1, (int) MfVersion,
    1, (int) MfDescrip,
    1, (int) vdcType,
    1, (int) IntPrec,
    1, (int) RealPrec,
    1, (int) IndexPrec,
    1, (int) ColPrec,
    1, (int) CIndPrec,
    1, (int) MaxCInd,
    1, (int) CVExtent,
    1, (int) MfElList,
    1, (int) FontList,
    1, (int) CharAnnounce,
    2, (int) ScalMode,
    2, (int) ColSelMode,
    2, (int) LWidSpecMode,
    2, (int) MarkSizSpecMode,
    2, (int) vdcExtent,
    2, (int) BackCol,
    3, (int) vdcIntPrec,
    3, (int) Transp,
    3, (int) ClipRect,
    3, (int) ClipIndic,
    4, (int) PolyLine,
    4, (int) PolyMarker,
    4, (int) Text,
    4, (int) C_Polygon,
    4, (int) Cell_Array,
    5, (int) LType,
    5, (int) LWidth,
    5, (int) LColour,
    5, (int) MType,
    5, (int) MSize,
    5, (int) MColour,
    5, (int) TFIndex,
    5, (int) TPrec,
    5, (int) CExpFac,
    5, (int) CSpace,
    5, (int) TColour,
    5, (int) CHeight,
    5, (int) COrient,
    5, (int) TPath,
    5, (int) TAlign,
    5, (int) IntStyle,
    5, (int) FillColour,
    5, (int) HatchIndex,
    5, (int) PatIndex,
    5, (int) ColTab
};
