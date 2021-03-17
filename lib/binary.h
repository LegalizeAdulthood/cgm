#pragma once

#include "stream.h"

namespace cgm
{

class BinaryMetafileWriter : public MetafileStreamWriter
{
public:
    explicit BinaryMetafileWriter(std::ostream &stream);
    explicit BinaryMetafileWriter(int fd);

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
    void scalingMode(ScalingMode mode, float value) override;
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
    void cellArray( Point<int> c1, Point<int> c2, Point<int> c3, int colorPrecision, int nx, int ny, const int* colors ) override;
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

private:
    void outByte(char chr);
    void cgmb_start_cmd(int cl, int el);
    void cgmb_flush_cmd(int this_flush);
    void cgmb_out_bc(int c);
    void cgmb_out_bs(const char *cptr, int n);
    void cgmb_string(const char *cptr, int slen);
    void cgmb_gint(int xin, int precision);
    void cgmb_uint(unsigned xin, int precision);
    void cgmb_fixed(double xin);
    void cgmb_float(double xin);
    void cgmb_dcint(int xin);
    void cgmb_vint(int xin);
    void cgmb_sint(int xin);
    void cgmb_xint(int xin);
    void cgmb_cxint(int xin);
    void cgmb_eint(int xin);

    char m_cmdBuffer[hdr_long + max_long]{}; /* where we buffer output */
    char *m_cmdHdr{};                        /* the command header */
    char *m_cmdData{};                       /* the command data */
    int m_cmdIndex{};                        /* index into the command data */
    int m_buffIndex{};                       /* index into the buffer */
    int m_partition{};                       /* which partition in the output */
};

}        // namespace cgm
