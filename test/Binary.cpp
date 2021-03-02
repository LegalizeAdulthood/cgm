#include <cgm/cgm.h>

#include <catch2/catch.hpp>

#include <array>
#include <cstdint>
#include <sstream>

namespace
{

template <typename T, int N>
int numOf(T (&ary)[N])
{
    return N;
}

enum ElementClass
{
    Delimiter = 0,
    MetafileDescriptor = 1,
    PictureDescriptor = 2,
    Control = 3,
    Primitive = 4,
    Attribute = 5,
    Escape = 6,
    External = 7,
    Segment = 8,
    Application = 9
};

enum ElementOpCode
{
    NoOp = 0,
    BeginMetafile = 1,
    EndMetafile = 2,
    BeginPicture = 3,
    BeginPictureBody = 4,
    EndPicture = 5,
    BeginSegment = 6,
    EndSegment = 7,
    BeginFigure = 8,
    EndFigure = 9,
    BeginProtection = 13,
    EndProtection = 14,
    BeginCompoundLine = 15,
    EndCompoundLine = 16,
    BeginCompoundTextPath = 17,
    EndCompoundTextPath = 18,
    BeginTileArray = 19,
    EndTileArray = 20,
    BeginAppStruct = 21,
    BeginAppStructBody = 22,
    EndAppStruct = 23,
    MetafileVersion = 1,
    MetafileDescription = 2,
    VdcType = 3,
    IntegerPrecision = 4,
    RealPrecision = 5,
    IndexPrecision = 6,
    ColorPrecision = 7,
    ColorIndexPrecision = 8,
    MaximumColorIndex = 9,
    ColorValueExtent = 10,
    MetafileElementList = 11,
    MetafileDefaultsReplacement = 12,
    FontList = 13,
    CharacterSetList = 14,
    CharacterCodingAnnouncer = 15,
    NamePrecision = 16,
    MaximumVdcExtent = 17,
    SegmentPriorityExtent = 18,
    ColorModel = 19,
    ColorCalibration = 20,
    FontProperties = 21,
    GlyphMapping = 22,
    SymbolLibrariList = 23,
    PictureDirectory = 24,
    ScalingMode = 1,
    ColorMode = 2,
    LineWidthMode = 3,
    MarkerSizeMode = 4,
    EdgeWidthMode = 5,
    VdcExtent = 6,
    BackgroundColor = 7,
    DeviceViewport = 8,
    DeviceViewportMode = 9,
    DeviceViewportMapping = 10,
    LineRep = 11,
    MarkerRep = 12,
    TextRep = 13,
    FillRep = 14,
    EdgeRep = 15,
    InteriorStyleMode = 16,
    LineEdgeTypeDef = 17,
    HatchStyleDef = 18,
    PatternDef = 19,
    AppStructDir = 20,
    VdcIntPrec = 1,
    VdcRealPrec = 2,
    AuxiliaryColor = 3,
    Transparency = 4,
    ClipRect = 5,
    ClipIndicator = 6,
    LineClipMode = 7,
    MarkerClipMode = 8,
    EdgeClipMode = 9,
    NewRegion = 10,
    SavePrimContext = 11,
    RestorePrimContext = 12,
    ProtectionIndicator = 17,
    TextPathMode = 18,
    MitreLimit = 19,
    TransparentCellColor = 20,
    Polyline = 1,
    DisjointPolyline = 2,
    Polymarker = 3,
    Text = 4,
    RestrictedText = 5,
    AppendText = 6,
    Polygon = 7,
    PolygonSet = 8,
    CellArray = 9,
    GDP = 10,
    Rectangle = 11,
    Circle = 12,
    CircularArc3Point = 13,
    CircularArc3PointClose = 14,
    CircularArcCenter = 15,
    CircularArcCenterClose = 16,
    Ellipse = 17,
    EllipticalArc = 18,
    EllipticalArcClose = 19,
    CircularArcCenterReversed = 20,
    ConnectingEdge = 21,
    HyperbolicArc = 22,
    ParabolicArc = 23,
    NUBSpline = 24,
    NURBSpline = 25,
    Polybezier = 26,
    Polysymbol = 27,
    BitonalTile = 28,
    Tile = 29,
    LineIndex = 1,
    LineType = 2,
    LineWidth = 3,
    LineColor = 4,
    MarkerIndex = 5,
    MarkerType = 6,
    MarkerSize = 7,
    MarkerColor = 8,
    TextIndex = 9,
    TextFontIndex = 10,
    TextPrecision = 11,
    CharExpansion = 12,
    CharSpacing = 13,
    TextColor = 14,
    CharHeight = 15,
    CharOrientation = 16,
    TextPath = 17,
    TextAlignment = 18,
    CharSetIndex = 19,
    AltCharSetIndex = 20,
    FillIndex = 21,
    InteriorStyle = 22,
    FillColor = 23,
    HatchIndex = 24,
    PatternIndex = 25,
    EdgeIndex = 26,
    EdgeType = 27,
    EdgeColor = 29,
    EdgeVisibility = 30,
    FillRefPt = 31,
    PatternTable = 32,
    PatternSize = 33,
    ColorTable = 34,
    AspectSourceFlags = 35,
    PickId = 36,
    LineCap = 37,
    LineJoin = 38,
    LineTypeContinuation = 39,
    LineTypeInitialOffset = 40,
    TextScoreType = 41,
    RestrictedTextType = 42,
    InterpolatedInterior = 43,
    EdgeCap = 44,
    EdgeJoin = 45,
    EdgeTypeContinuation = 46,
    EdgeTypeInitialOffset = 47,
    SymbolLibraryIndex = 48,
    SymbolColor = 49,
    SymbolSize = 50,
    SymbolOrientation = 51,
};

struct OpCode
{
    int classCode;
    int opCode;
    int paramLength;
};

bool operator==(OpCode lhs, OpCode rhs)
{
    return lhs.classCode == rhs.classCode
        && lhs.opCode == rhs.opCode
        && lhs.paramLength == rhs.paramLength;
}

std::ostream &operator<<(std::ostream &stream, OpCode value)
{
    return stream << '{' << value.classCode << ", " << value.opCode << ", " << value.paramLength << '}';
}

OpCode header(const std::string &str)
{
    const auto first = static_cast<unsigned char>(str[0]);
    const auto second = static_cast<unsigned char>(str[1]);
    return OpCode{
        first >> 4 & 0xF,
        (first & 0xF) << 3 | (second & 0xE0) >> 5,
        second & 0x1F
    };
}

std::string unpack(const std::string &str, int offset)
{
    return str.substr(offset + 1, str[offset]);
}

int i8(const std::string &str, int offset)
{
    return static_cast<int>(str[offset]);
}

int u8(const std::string &str, int offset)
{
    return static_cast<unsigned char>(str[offset]);
}

int i16(const std::string &str, int offset)
{
    // This is x86 endian
    const char bytes[2]{str[offset+1], str[offset]};
    return *reinterpret_cast<const std::int16_t *>(bytes);
}

float f32(const std::string &str, int offset)
{
    const char bytes[4]{str[offset], str[offset+1], str[offset+2], str[offset+3]};
    return *reinterpret_cast<const float *>(bytes);
}

const int expectedMetafileElementList[] = {
    54,
    Delimiter, BeginMetafile,
    Delimiter, EndMetafile,
    Delimiter, BeginPicture,
    Delimiter, BeginPictureBody,
    Delimiter, EndPicture,
    MetafileDescriptor, MetafileVersion,
    MetafileDescriptor, MetafileDescription,
    MetafileDescriptor, VdcType,
    MetafileDescriptor, IntegerPrecision,
    MetafileDescriptor, RealPrecision,
    MetafileDescriptor, IndexPrecision,
    MetafileDescriptor, ColorPrecision,
    MetafileDescriptor, ColorIndexPrecision,
    MetafileDescriptor, MaximumColorIndex,
    MetafileDescriptor, ColorValueExtent,
    MetafileDescriptor, MetafileElementList,
    MetafileDescriptor, FontList,
    MetafileDescriptor, CharacterCodingAnnouncer,
    PictureDescriptor, ScalingMode,
    PictureDescriptor, ColorMode,
    PictureDescriptor, LineWidthMode,
    PictureDescriptor, MarkerSizeMode,
    PictureDescriptor, VdcExtent,
    PictureDescriptor, BackgroundColor,
    Control, VdcIntPrec,
    Control, Transparency,
    Control, ClipRect,
    Control, ClipIndicator,
    Primitive, Polyline,
    Primitive, Polymarker,
    Primitive, Text,
    Primitive, Polygon,
    Primitive, CellArray,
    Attribute, LineType,
    Attribute, LineWidth,
    Attribute, LineColor,
    Attribute, MarkerType,
    Attribute, MarkerSize,
    Attribute, MarkerColor,
    Attribute, TextFontIndex,
    Attribute, TextPrecision,
    Attribute, CharExpansion,
    Attribute, CharSpacing,
    Attribute, TextColor,
    Attribute, CharHeight,
    Attribute, CharOrientation,
    Attribute, TextPath,
    Attribute, TextAlignment,
    Attribute, InteriorStyle,
    Attribute, FillColor,
    Attribute, HatchIndex,
    Attribute, PatternIndex,
    Attribute, ColorTable
};

}

TEST_CASE("binary encoding")
{
    std::ostringstream stream;
    std::unique_ptr<cgm::MetafileWriter> writer{create(stream, cgm::Encoding::Binary)};

    SECTION("begin metafile")
    {
        const char ident[]{"cgm unit test"};
        writer->beginMetafile(ident);

        const std::string str = stream.str();
        const int encodedLength = numOf(ident) - 1;
        REQUIRE(str.size() == std::size_t(3 + encodedLength));
        const char *data = str.data();
        REQUIRE(header(str) == OpCode{Delimiter, BeginMetafile, numOf(ident)});
        REQUIRE(int(data[2]) == encodedLength);
        REQUIRE(unpack(str, 2) == "cgm unit test"); 
    }
    SECTION("end metafile")
    {
        writer->endMetafile();

        const std::string str = stream.str();
        REQUIRE(header(str) == OpCode{Delimiter, EndMetafile, 0});
    }
    SECTION("begin picture")
    {
        const char ident[]{"cgm unit test"};
        writer->beginPicture(ident);

        const std::string str = stream.str();
        const int encodedLength = numOf(ident) - 1;
        REQUIRE(str.size() == std::size_t(3 + encodedLength));
        const char *data = str.data();
        REQUIRE(header(str) == OpCode{Delimiter, BeginPicture, numOf(ident)});
        REQUIRE(int(data[2]) == encodedLength);
        REQUIRE(str.substr(3) == "cgm unit test");
    }
    SECTION("begin picture body")
    {
        writer->beginPictureBody();

        const std::string str = stream.str();
        REQUIRE(header(str) == OpCode{Delimiter, BeginPictureBody, 0});
    }
    SECTION("end picture")
    {
        writer->endPicture();

        const std::string str = stream.str();
        REQUIRE(header(str) == OpCode{Delimiter, EndPicture, 0});
    }
    SECTION("metafile version")
    {
        writer->metafileVersion(2);

        const std::string str = stream.str();
        REQUIRE(header(str) == OpCode{MetafileDescriptor, MetafileVersion, 2});
        REQUIRE(i16(str, 2) == 2);
    }
    SECTION("metafile description")
    {
        const char ident[]{"this is a foo widget"};
        writer->metafileDescription(ident);

        const std::string str = stream.str();
        REQUIRE(header(str) == OpCode{MetafileDescriptor, MetafileDescription, numOf(ident)});
        REQUIRE(unpack(str, 2) == ident);
    }
    SECTION("vdc type integer")
    {
        writer->vdcType(cgm::VdcType::Integer);

        const std::string str = stream.str();
        REQUIRE(header(str) == OpCode{MetafileDescriptor, VdcType, 2});
        REQUIRE(i16(str, 2) == 0);
    }
    SECTION("vdc type real")
    {
        writer->vdcType(cgm::VdcType::Real);

        const std::string str = stream.str();
        REQUIRE(header(str) == OpCode{MetafileDescriptor, VdcType, 2});
        REQUIRE(i16(str, 2) == 1);
    }
    SECTION("integer precision")
    {
        writer->intPrecisionBinary(32);

        const std::string str = stream.str();
        REQUIRE(header(str) == OpCode{MetafileDescriptor, IntegerPrecision, 2});
        REQUIRE(i16(str, 2) == 32);
    }
    SECTION("real precision")
    {
        writer->realPrecisionBinary(cgm::RealPrecision::Floating, 9, 23);

        const std::string str = stream.str();
        REQUIRE(header(str) == OpCode{MetafileDescriptor, RealPrecision, 6});
        REQUIRE(i16(str, 2) == 0);
        REQUIRE(i16(str, 4) == 9);
        REQUIRE(i16(str, 6) == 23);
    }
    SECTION("index precision")
    {
        writer->indexPrecisionBinary(16);

        const std::string str = stream.str();
        REQUIRE(header(str) == OpCode{MetafileDescriptor, IndexPrecision, 2});
        REQUIRE(i16(str, 2) == 16);
    }
    SECTION("color precision")
    {
        writer->colorPrecisionBinary(16);

        const std::string str = stream.str();
        REQUIRE(header(str) == OpCode{MetafileDescriptor, ColorPrecision, 2});
        REQUIRE(i16(str, 2) == 16);
    }
    SECTION("color index precision")
    {
        writer->colorIndexPrecisionBinary(8);

        const std::string str = stream.str();
        REQUIRE(header(str) == OpCode{MetafileDescriptor, ColorIndexPrecision, 2});
        REQUIRE(i16(str, 2) == 8);
    }
    SECTION("maximum color index")
    {
        writer->maximumColorIndex(63);

        const std::string str = stream.str();
        REQUIRE(header(str) == OpCode{MetafileDescriptor, MaximumColorIndex, 2});
        REQUIRE(i16(str, 2) == 63);
    }
    SECTION("color value extent")
    {
        writer->colorValueExtent(0, 63, 2, 31, 4, 15);

        const std::string str = stream.str();
        REQUIRE(header(str) == OpCode{MetafileDescriptor, ColorValueExtent, 6});
        REQUIRE(i8(str, 2) == 0);
        REQUIRE(i8(str, 3) == 2);
        REQUIRE(i8(str, 4) == 4);
        REQUIRE(i8(str, 5) == 63);
        REQUIRE(i8(str, 6) == 31);
        REQUIRE(i8(str, 7) == 15);
    }
    SECTION("metafile element list")
    {
        writer->metafileElementList();

        const std::string str = stream.str();
        // 31 bytes?  Seems wrong....
        const int paramLength = 31;
        REQUIRE(header(str) == OpCode{MetafileDescriptor, MetafileElementList, paramLength});
        int offset = 4;
        for (int value : expectedMetafileElementList)
        {
            REQUIRE(i16(str, offset) == value);
            offset += 2;
        }
    }
    // TODO: implement this?
    //SECTION("metafile defaults replacement")
    //{
    //    writer->metafileDefaultsReplacement();

    //    REQUIRE(stream.str() == "BegMFDefaults;\n"
    //        "EndMFDefaults;\n");
    //}
    SECTION("font list")
    {
        std::vector<std::string> fonts{"Hershey Simplex", "Hershey Roman"};
        writer->fontList(fonts);

        const std::string str = stream.str();
        const int paramLength = 30;
        REQUIRE(header(str) == OpCode{MetafileDescriptor, FontList, paramLength});
        // TODO: is this right?  Shouldn't it be 2 length encoded strings?
        REQUIRE(unpack(str, 2) == "Hershey Simplex Hershey Roman");
    }
    // character set list
    SECTION("character coding announcer")
    {
        writer->characterCodingAnnouncer(cgm::CharCodeAnnouncer::Basic8Bit);

        const std::string str = stream.str();
        REQUIRE(header(str) == OpCode{MetafileDescriptor, CharacterCodingAnnouncer, 2});
        REQUIRE(i16(str, 2) == static_cast<int>(cgm::CharCodeAnnouncer::Basic8Bit));
    }
    SECTION("scaling mode")
    {
        SECTION("abstract")
        {
            writer->scaleMode(cgm::ScaleMode::Abstract, 1.0f);

            const std::string str = stream.str();
            REQUIRE(header(str) == OpCode{PictureDescriptor, ScalingMode, 6});
            REQUIRE(i16(str, 2) == static_cast<int>(cgm::ScaleMode::Abstract));
            // TODO: decode 16.16 fixed-point as float
            // REQUIRE(f32(str, 4) == 1.0f);
        }
        SECTION("metric")
        {
            writer->scaleMode(cgm::ScaleMode::Metric, 1.0f);

            const std::string str = stream.str();
            REQUIRE(header(str) == OpCode{PictureDescriptor, ScalingMode, 6});
            REQUIRE(i16(str, 2) == static_cast<int>(cgm::ScaleMode::Metric));
            // TODO: decode 16.16 fixed-point as float
            //REQUIRE(f32(str, 4) == 1.0f);
        }
        SECTION("color selection mode")
        {
            SECTION("indexed")
            {
                writer->colorMode(cgm::ColorMode::Indexed);

                const std::string str = stream.str();
                REQUIRE(header(str) == OpCode{PictureDescriptor, ColorMode, 2});
                REQUIRE(i16(str, 2) == static_cast<int>(cgm::ColorMode::Indexed));
            }
            SECTION("direct")
            {
                writer->colorMode(cgm::ColorMode::Direct);

                const std::string str = stream.str();
                REQUIRE(header(str) == OpCode{PictureDescriptor, ColorMode, 2});
                REQUIRE(i16(str, 2) == static_cast<int>(cgm::ColorMode::Direct));
            }
        }
        SECTION("line width specification mode")
        {
            SECTION("absolute")
            {
                writer->lineWidthMode(cgm::LineWidthMode::Absolute);

                const std::string str = stream.str();
                REQUIRE(header(str) == OpCode{PictureDescriptor, LineWidthMode, 2});
                REQUIRE(i16(str, 2) == static_cast<int>(cgm::LineWidthMode::Absolute));
            }
            SECTION("scaled")
            {
                writer->lineWidthMode(cgm::LineWidthMode::Scaled);

                const std::string str = stream.str();
                REQUIRE(header(str) == OpCode{PictureDescriptor, LineWidthMode, 2});
                REQUIRE(i16(str, 2) == static_cast<int>(cgm::LineWidthMode::Scaled));
            }
        }
        SECTION("marker size specification mode")
        {
            SECTION("absolute")
            {
                writer->markerSizeMode(cgm::MarkerSizeMode::Absolute);

                const std::string str = stream.str();
                REQUIRE(header(str) == OpCode{PictureDescriptor, MarkerSizeMode, 2});
                REQUIRE(i16(str, 2) == static_cast<int>(cgm::MarkerSizeMode::Absolute));
            }
            SECTION("scaled")
            {
                writer->markerSizeMode(cgm::MarkerSizeMode::Scaled);

                const std::string str = stream.str();
                REQUIRE(header(str) == OpCode{PictureDescriptor, MarkerSizeMode, 2});
                REQUIRE(i16(str, 2) == static_cast<int>(cgm::MarkerSizeMode::Scaled));
            }
        }
    }
}

TEST_CASE("TODO", "[.]")
{
    std::ostringstream stream;
    std::unique_ptr<cgm::MetafileWriter> writer{create(stream, cgm::Encoding::Binary)};

    // edge width specification mode
    SECTION("vdc extent")
    {
        writer->vdcExtent(0, 0, 640, 480);

        REQUIRE(stream.str() == "VDCExt 0,0 640,480;\n");
    }
    SECTION("background color")
    {
        writer->backgroundColor(128, 64, 128);

        REQUIRE(stream.str() == "BackColr 128 64 128;\n");
    }
    SECTION("vdc integer precision")
    {
        writer->vdcIntegerPrecision(-128, 128);

        REQUIRE(stream.str() == "VDCIntegerPrec -128 128;\n");
    }
    // vdc real precision
    // auxiliary color
    // transparency
    SECTION("clip rectangle")
    {
        writer->clipRectangle(20, 20, 64, 64);

        REQUIRE(stream.str() == "ClipRect 20,20 64,64;\n");
    }
    SECTION("clip indicator")
    {
        writer->clipIndicator(true);

        REQUIRE(stream.str() == "Clip On;\n");
    }
    SECTION("polyline")
    {
        const std::vector<cgm::Point<int>> points{{10, 10}, {20, 20}};

        writer->polyline(points);

        REQUIRE(stream.str() == "Line 10,10 20,20;\n");
    }
    // disjoint polyline
    SECTION("polymarker")
    {
        const std::vector<cgm::Point<int>> points{{10, 10}, {20, 20}};

        writer->polymarker(points);

        REQUIRE(stream.str() == "Marker 10,10 20,20;\n");
    }
    SECTION("text")
    {
        writer->text({10, 10}, cgm::TextFlag::Final, "Hello, world!");

        REQUIRE(stream.str() == "Text 10,10 Final \"Hello, world!\";\n");
    }
    // restricted text
    // append text
    SECTION("polygon")
    {
        const std::vector<cgm::Point<int>> points{{10, 10}, {20, 10}, {20, 20}, {10, 20}};

        writer->polygon(points);

        REQUIRE(stream.str() == "Polygon 10,10 20,10 20,20 10,20;\n");
    }
    // polygon set
    SECTION("cell array")
    {
        std::array<int, 16> cellArray{0, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 0};

        writer->cellArray({0, 0}, {10, 0}, {10, 10}, 4, 4, cellArray.data());
    }
    // generalized drawing primitive
    // rectangle
    // circle
    // circular arc 3 point
    // circular arc 3 point close
    // circular arc center
    // circular arc center close
    // ellipse
    // elliptical arc
    // elliptical arc close
    // line bundle index
    SECTION("line type")
    {
        writer->lineType(1);

        REQUIRE(stream.str() == "LineType 1;\n");
    }
    SECTION("line width")
    {
        writer->lineWidth(1.0f);

        REQUIRE(stream.str() == "LineWidth 1.000000;\n");
    }
    // line color
    SECTION("line color")
    {
        writer->lineColor(6);

        REQUIRE(stream.str() == "LineColr 6;\n");
    }
    // marker bundle index
    SECTION("marker type")
    {
        writer->markerType(6);

        REQUIRE(stream.str() == "MarkerType 6;\n");
    }
    // marker size
    SECTION("marker size")
    {
        writer->markerSize(0.1f);

        REQUIRE(stream.str() == "MarkerSize 0.100000;\n");
    }
    SECTION("marker color")
    {
        writer->markerColor(6);

        REQUIRE(stream.str() == "MarkerColr 6;\n");
    }
    // text bundle index
    SECTION("text font index")
    {
        writer->textFontIndex(6);

        REQUIRE(stream.str() == "TextFontIndex 6;\n");
    }
    SECTION("text precision")
    {
        SECTION("string")
        {
            writer->textPrecision(cgm::TextPrecision::String);

            REQUIRE(stream.str() == "TextPrec String;\n");
        }
        SECTION("character")
        {
            writer->textPrecision(cgm::TextPrecision::Character);

            REQUIRE(stream.str() == "TextPrec Character;\n");
        }
        SECTION("stroke")
        {
            writer->textPrecision(cgm::TextPrecision::Stroke);

            REQUIRE(stream.str() == "TextPrec Stroke;\n");
        }
    }
    SECTION("character expansion factor")
    {
        writer->charExpansion(0.1f);

        REQUIRE(stream.str() == "CharExpan 0.100000;\n");
    }
    SECTION("character spacing")
    {
        writer->charSpacing(0.1f);

        REQUIRE(stream.str() == "CharSpace 0.100000;\n");
    }
    SECTION("text color")
    {
        writer->textColor(6);

        REQUIRE(stream.str() == "TextColr 6;\n");
    }
    SECTION("character height")
    {
        writer->charHeight(12);

        REQUIRE(stream.str() == "CharHeight 12;\n");
    }
    SECTION("character orientation")
    {
        writer->charOrientation(0, 1, 1, 0);

        REQUIRE(stream.str() == "CharOri 0 1 1 0;\n");
    }
    SECTION("text path")
    {
        SECTION("right")
        {
            writer->textPath(cgm::TextPath::Right);

            REQUIRE(stream.str() == "TextPath Right;\n");
        }
        SECTION("left")
        {
            writer->textPath(cgm::TextPath::Left);

            REQUIRE(stream.str() == "TextPath Left;\n");
        }
        SECTION("up")
        {
            writer->textPath(cgm::TextPath::Up);

            REQUIRE(stream.str() == "TextPath Up;\n");
        }
        SECTION("down")
        {
            writer->textPath(cgm::TextPath::Down);

            REQUIRE(stream.str() == "TextPath Down;\n");
        }
    }
    SECTION("text alignment")
    {
        SECTION("normal, normal")
        {
            writer->textAlignment(cgm::HorizAlign::Normal, cgm::VertAlign::Normal, 0.1f, 0.2f);

            REQUIRE(stream.str() == "TextAlign NormHoriz NormVert 0.100000 0.200000;\n");
        }
        SECTION("left, top")
        {
            writer->textAlignment(cgm::HorizAlign::Left, cgm::VertAlign::Top, 0.0f, 0.0f);

            REQUIRE(stream.str() == "TextAlign Left Top 0.000000 0.000000;\n");
        }
        SECTION("center, cap")
        {
            writer->textAlignment(cgm::HorizAlign::Center, cgm::VertAlign::Cap, 0.0f, 0.0f);

            REQUIRE(stream.str() == "TextAlign Ctr Cap 0.000000 0.000000;\n");
        }
        SECTION("right, half")
        {
            writer->textAlignment(cgm::HorizAlign::Right, cgm::VertAlign::Half, 0.0f, 0.0f);

            REQUIRE(stream.str() == "TextAlign Right Half 0.000000 0.000000;\n");
        }
        SECTION("continuous, base")
        {
            writer->textAlignment(cgm::HorizAlign::Continuous, cgm::VertAlign::Base, 0.0f, 0.0f);

            REQUIRE(stream.str() == "TextAlign ContHoriz Base 0.000000 0.000000;\n");
        }
        SECTION("normal, bottom")
        {
            writer->textAlignment(cgm::HorizAlign::Normal, cgm::VertAlign::Bottom, 0.0f, 0.0f);

            REQUIRE(stream.str() == "TextAlign NormHoriz Bottom 0.000000 0.000000;\n");
        }
        SECTION("normal, continuous")
        {
            writer->textAlignment(cgm::HorizAlign::Normal, cgm::VertAlign::Continuous, 0.0f, 0.0f);

            REQUIRE(stream.str() == "TextAlign NormHoriz ContVert 0.000000 0.000000;\n");
        }
    }
    // character set index
    // alternate character set index
    // full bundle index
    SECTION("interior style")
    {
        SECTION("hollow")
        {
            writer->interiorStyle(cgm::InteriorStyle::Hollow);

            REQUIRE(stream.str() == "IntStyle Hollow;\n");
        }
        SECTION("solid")
        {
            writer->interiorStyle(cgm::InteriorStyle::Solid);

            REQUIRE(stream.str() == "IntStyle Solid;\n");
        }
        SECTION("pattern")
        {
            writer->interiorStyle(cgm::InteriorStyle::Pattern);

            REQUIRE(stream.str() == "IntStyle Pat;\n");
        }
        SECTION("hatch")
        {
            writer->interiorStyle(cgm::InteriorStyle::Hatch);

            REQUIRE(stream.str() == "IntStyle Hatch;\n");
        }
        SECTION("empty")
        {
            writer->interiorStyle(cgm::InteriorStyle::Empty);

            REQUIRE(stream.str() == "IntStyle Empty;\n");
        }
    }
    SECTION("fill color")
    {
        writer->fillColor(6);

        REQUIRE(stream.str() == "FillColr 6;\n");
    }
    SECTION("hatch index")
    {
        writer->hatchIndex(6);

        REQUIRE(stream.str() == "HatchIndex 6;\n");
    }
    SECTION("pattern index")
    {
        writer->patternIndex(6);

        REQUIRE(stream.str() == "PatIndex 6;\n");
    }
    // edge bundle index
    // edge type
    // edge width
    // edge color
    // edge visibility
    // fill reference point
    // pattern table
    // pattern size
    SECTION("color table")
    {
        std::vector<cgm::Color> colors{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}};
        writer->colorTable(6, colors);

        REQUIRE(stream.str() == "ColrTable 6 0 0 0 255 0 0 0 255 0 0 0 255 255 255 255;\n");
    }
    // aspect source flags
    // escape
    // message
    // application data
    //----- Addendum 1
    // begin segment
    // end segment
    // name precision
    // maximum vdc extent
    // segment priority extent
    // device viewport
    // device viewport specification mode
    // device viewport mapping
    // line representation
    // marker representation
    // text representation
    // fill representation
    // edge representation
    // line clipping mode
    // marker clipping mode
    // edge clipping mode
    // begin figure
    // end figure
    // new region
    // save primitive context
    // restore primitive context
    // circular arc center reversed
    // connecting edge
    // pick identifier
    // copy segment
    // inheritance filter
    // clip inheritance
    // segment transformation
    // segment highlighting
    // segment display priority
    // segment pick priority
}
