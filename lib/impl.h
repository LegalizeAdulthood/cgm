#pragma once

#define MAX_COLOR 1256  /* maximum number of predefined colors */

#define cprec		    8		    /* bits used for color precision */
#define cxprec  	    16		    /*     and color index precision */
#define max_colors	    (01<<cprec)	    /* number of addressable colors */

#define max_pbuffer	    512		    /* number of points in buffer */

#define max_std_linetype    4		    /* number of std. linetypes */
#define max_std_markertype  5		    /* number of std. markertypes */
#define max_std_textfont    32		    /* number of std. textfonts */

#define max_coord	    32767	    /* max value of vdc coordinates */

#define n_melements         54		    /* number of metafile elements */

#define int_flush	    0		    /* intermediate flush */
#define final_flush	    1		    /* final flush */

#define real_prec_exp	    16		    
#define real_prec_fract	    16

#define max_pwrs	    8
#define cgmt_recl	    78
#define max_buffer	    500

#define byte_size	    8
#define byte_mask	    255

#define max_long	    10240	    /* longest partition of command 
					       data (even) */
#define hdr_long	    4		    /* 4 bytes for the command header */
#define hdr_short	    2		    /* a short command header length */
#define max_short	    30		    /* short form data length */


/* attributes */

enum 	vdc_enum    {vdc_int, vdc_real};
enum	cs_enum	    {indexed_color_mode, direct_color_mode};
enum 	spec_enum   {absolute, scaled}; 
enum 	bool_enum   {off, on};
enum 	line_enum   {solid_l = 1, dash, dot_l, dash_dot, dash_d_d};
enum 	txt_enum    {string, character, stroke};
enum	path_enum   {right, left, up, down};
enum	hor_align   {normal_h, left_h, center_h, right_h, cont_h };
enum	ver_align   {normal_v, top_v, cap_v, half_v, base_v, bottom_v, cont_v };
enum 	is_enum	    {hollow, solid_i, pattern, hatch, empty};

/* used cgm functions */

enum enum_cgm_fun {
    begin, end, bp, bpage, epage, mfversion, mfdescrip, vdctype, intprec,
    realprec, indexprec, colprec, cindprec, cvextent, maxcind, mfellist,
    fontlist, cannounce, scalmode, colselmode, lwsmode, msmode, vdcextent,
    backcol, vdcintprec, cliprect, clipindic, pline, pmarker, text, pgon,
    ltype, lwidth, lcolour, mtype, msize, mcolour, tfindex, tprec, cexpfac,
    cspace, tcolour, cheight, corient, tpath, talign, intstyle, fillcolour,
    hindex, pindex, coltab, carray };

enum encode_enum { cgm_binary, cgm_character, cgm_clear_text, cgm_grafkit };

/* class 0, delimiter elements */

enum cgm_class_0 {
    No_Op = 0, B_Mf, E_Mf, B_Pic, B_Pic_Body, E_Pic };


/* class 1, metafile descriptor elements */

enum cgm_class_1 {
    MfVersion = 1, MfDescrip, vdcType, IntPrec, RealPrec, IndexPrec, ColPrec, 
    CIndPrec, MaxCInd, CVExtent, MfElList, MfDefRep, FontList, CharList, 
    CharAnnounce };


/* class 2, page descriptor elements */

enum cgm_class_2 {
    ScalMode = 1, ColSelMode, LWidSpecMode, MarkSizSpecMode, EdWidSpecMode, 
    vdcExtent, BackCol };


/* class 3, control elements */

enum cgm_class_3 {
    vdcIntPrec = 1, vdcRPrec, AuxCol, Transp, ClipRect, ClipIndic };


/* class 4, graphical primitives */

enum cgm_class_4 {
    PolyLine = 1, Dis_Poly, PolyMarker, Text, Rex_Text, App_Text, C_Polygon, 
    Poly_Set, Cell_Array, Gen_D_Prim, C_Rectangle, Cgm_Circle, Circ_3, 
    Circ_3_Close, Circ_Centre, Circ_C_Close, C_Ellipse, Ellip_Arc,
    El_Arc_Close };


/* class 5, attribute elements */

enum cgm_class_5 { 
    LBIndex = 1,  LType,  LWidth,  LColour,  MBIndex,  MType,  MSize,  MColour,
    TBIndex, TFIndex, TPrec, CExpFac,  CSpace, TColour, CHeight,  COrient,  
    TPath, TAlign, CSetIndex, AltCSetIndex, FillBIndex, IntStyle, FillColour,
    HatchIndex,  PatIndex, EdBIndex, EType, EdWidth, EdColour, EdVis, FillRef,
    PatTab,  PatSize, ColTab,  AspsFlags };


/* class 6, escape element */

enum cgm_class_6 {
    C_Escape = 1 };


/* class 7, external elements */

enum cgm_class_7 {
    Message = 1, Ap_Data };


/* metafile elements */
extern const char *const cgmt_delim[];

/* metafile descriptors */
extern const char *const cgmt_mfdesc[];

/* page descriptors */
extern const char *const cgmt_pdesc[];

/* control elements */
extern const char *const cgmt_control[];

/* graphical primitives */
extern const char *const cgmt_gprim[];

/* attribute elements */
extern const char *const cgmt_attr[];

/* escape element */
extern const char *const cgmt_escape[];

/* external elements */
extern const char *const cgmt_external[];

extern const char *const *const cgmt_cptr[8];

/* used metafile elements */
extern const int element_list[n_melements*2];
