#include <cgm/cgm.h>

#include <catch2/catch.hpp>

#include <array>
#include <cstdint>
#include <numeric>
#include <sstream>

namespace
{

template <typename T, int N>
int numOf(T (&ary)[N])
{
    return N;
}

template <typename T, size_t N>
size_t arraySize(T (&ary)[N])
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
    VdcIntegerPrecision = 1,
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

OpCode header(const std::string &str, int offset = 0)
{
    const auto first = static_cast<unsigned char>(str[offset]);
    const auto second = static_cast<unsigned char>(str[offset + 1]);
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

int i24(const std::string &str, int offset)
{
    const char bytes[4]{str[offset+2], str[offset+1], str[offset], 0};
    return *reinterpret_cast<const std::int32_t *>(bytes);
}

int i32(const std::string &str, int offset)
{
    const char bytes[4]{str[offset+3], str[offset+2], str[offset+1], str[offset]};
    return *reinterpret_cast<const std::int32_t *>(bytes);
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
    Control, VdcIntegerPrecision,
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
    const int pad1 = 1;
    const int headerLen = 2;

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
        REQUIRE(str.size() == 2);
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
        REQUIRE(str.size() == 2);
        REQUIRE(header(str) == OpCode{Delimiter, BeginPictureBody, 0});
    }
    SECTION("end picture")
    {
        writer->endPicture();

        const std::string str = stream.str();
        REQUIRE(str.size() == 2);
        REQUIRE(header(str) == OpCode{Delimiter, EndPicture, 0});
    }
    SECTION("metafile version")
    {
        writer->metafileVersion(2);

        const std::string str = stream.str();
        REQUIRE(str.size() == headerLen + 2);
        REQUIRE(header(str) == OpCode{MetafileDescriptor, MetafileVersion, 2});
        REQUIRE(i16(str, 2) == 2);
    }
    SECTION("metafile description")
    {
        const char ident[]{"this is a foo widget"};
        writer->metafileDescription(ident);

        const std::string str = stream.str();
        REQUIRE(str.size() == headerLen + arraySize(ident) + pad1);
        REQUIRE(header(str) == OpCode{MetafileDescriptor, MetafileDescription, numOf(ident)});
        REQUIRE(unpack(str, 2) == ident);
    }
    SECTION("vdc type integer")
    {
        writer->vdcType(cgm::VdcType::Integer);

        const std::string str = stream.str();
        REQUIRE(str.size() == headerLen + 2);
        REQUIRE(header(str) == OpCode{MetafileDescriptor, VdcType, 2});
        REQUIRE(i16(str, 2) == 0);
    }
    SECTION("vdc type real")
    {
        writer->vdcType(cgm::VdcType::Real);

        const std::string str = stream.str();
        REQUIRE(str.size() == headerLen + 2);
        REQUIRE(header(str) == OpCode{MetafileDescriptor, VdcType, 2});
        REQUIRE(i16(str, 2) == 1);
    }
    SECTION("integer precision")
    {
        writer->intPrecisionBinary(32);

        const std::string str = stream.str();
        REQUIRE(str.size() == headerLen + 2);
        REQUIRE(header(str) == OpCode{MetafileDescriptor, IntegerPrecision, 2});
        REQUIRE(i16(str, 2) == 32);
    }
    SECTION("integer precision changes size of subsequent encoded integers")
    {
        writer->intPrecisionBinary(32);
        writer->intPrecisionBinary(8);
        writer->intPrecisionBinary(16);
        writer->intPrecisionBinary(24);
        writer->intPrecisionBinary(32);

        const std::string str = stream.str();
        REQUIRE(str.size() == headerLen*5 + 2 + 4 + 2 + 2 + 4);
        REQUIRE(header(str) == OpCode{MetafileDescriptor, IntegerPrecision, 2});
        REQUIRE(i16(str, 2) == 32);
        REQUIRE(header(str, 4) == OpCode{MetafileDescriptor, IntegerPrecision, 4});
        REQUIRE(i32(str, 6) == 8);
        REQUIRE(header(str, 10) == OpCode{MetafileDescriptor, IntegerPrecision, 1});
        REQUIRE(i8(str, 12) == 16);
        REQUIRE(header(str, 14) == OpCode{MetafileDescriptor, IntegerPrecision, 2});
        REQUIRE(i16(str, 16) == 24);
        REQUIRE(header(str, 18) == OpCode{MetafileDescriptor, IntegerPrecision, 3});
        REQUIRE(i24(str, 20) == 32);
    }
    SECTION("real precision")
    {
        writer->realPrecisionBinary(cgm::RealPrecision::Floating, 9, 23);

        const std::string str = stream.str();
        REQUIRE(str.size() == headerLen + 6);
        REQUIRE(header(str) == OpCode{MetafileDescriptor, RealPrecision, 6});
        REQUIRE(i16(str, 2) == 0);
        REQUIRE(i16(str, 4) == 9);
        REQUIRE(i16(str, 6) == 23);
    }
    SECTION("index precision")
    {
        writer->indexPrecisionBinary(16);

        const std::string str = stream.str();
        REQUIRE(str.size() == headerLen + 2);
        REQUIRE(header(str) == OpCode{MetafileDescriptor, IndexPrecision, 2});
        REQUIRE(i16(str, 2) == 16);
    }
    SECTION("color precision")
    {
        writer->colorPrecisionBinary(16);

        const std::string str = stream.str();
        REQUIRE(str.size() == headerLen + 2);
        REQUIRE(header(str) == OpCode{MetafileDescriptor, ColorPrecision, 2});
        REQUIRE(i16(str, 2) == 16);
    }
    SECTION("color precision changes size of subsequent encoded colors")
    {
        writer->colorPrecisionBinary(16);
        writer->backgroundColor(10, 20, 30);

        const std::string str = stream.str();
        REQUIRE(str.size() == headerLen*2 + 2 + 3*2);
        REQUIRE(header(str) == OpCode{MetafileDescriptor, ColorPrecision, 2});
        REQUIRE(i16(str, 2) == 16);
        REQUIRE(header(str, 4) == OpCode{PictureDescriptor, BackgroundColor, 6});
        REQUIRE(i16(str, 6) == 10);
        REQUIRE(i16(str, 8) == 20);
        REQUIRE(i16(str, 10) == 30);
    }
    SECTION("color index precision")
    {
        writer->colorIndexPrecisionBinary(8);

        const std::string str = stream.str();
        REQUIRE(str.size() == headerLen + 2);
        REQUIRE(header(str) == OpCode{MetafileDescriptor, ColorIndexPrecision, 2});
        REQUIRE(i16(str, 2) == 8);
    }
    SECTION("color index precision changes size of subsequent encoded indices")
    {
        writer->colorIndexPrecisionBinary(32);
        writer->lineColor(655360);

        const std::string str = stream.str();
        REQUIRE(str.size() == headerLen*2 + 2 + 4);
        REQUIRE(header(str) == OpCode{MetafileDescriptor, ColorIndexPrecision, 2});
        REQUIRE(i16(str, 2) == 32);
        REQUIRE(header(str, 4) == OpCode{Attribute, LineColor, 4});
        REQUIRE(i32(str, 6) == 655360);
    }
    SECTION("maximum color index")
    {
        writer->maximumColorIndex(63);

        const std::string str = stream.str();
        REQUIRE(str.size() == headerLen + 2);
        REQUIRE(header(str) == OpCode{MetafileDescriptor, MaximumColorIndex, 1});
        REQUIRE(i8(str, 2) == 63);
    }
    SECTION("color value extent")
    {
        writer->colorValueExtent(0, 63, 2, 31, 4, 15);

        const std::string str = stream.str();
        REQUIRE(str.size() == headerLen + 6);
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
        REQUIRE(str.size() == headerLen + 2 + i16(str, 2));
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
        const int paramLength = static_cast<int>(fonts.size()) +
            std::accumulate(fonts.begin(), fonts.end(), 0, [](int sum, const std::string &str) { return sum + static_cast<int>(str.size()); });
        REQUIRE(str.size() == headerLen + paramLength);
        REQUIRE(header(str) == OpCode{MetafileDescriptor, FontList, paramLength});
        REQUIRE(unpack(str, 2) == "Hershey Simplex");
        REQUIRE(unpack(str, 2 + 1 + fonts[0].size()) == "Hershey Roman");
    }
    // character set list
    SECTION("character coding announcer")
    {
        writer->characterCodingAnnouncer(cgm::CharCodeAnnouncer::Basic8Bit);

        const std::string str = stream.str();
        REQUIRE(str.size() == headerLen + 2);
        REQUIRE(header(str) == OpCode{MetafileDescriptor, CharacterCodingAnnouncer, 2});
        REQUIRE(i16(str, 2) == static_cast<int>(cgm::CharCodeAnnouncer::Basic8Bit));
    }
    SECTION("scaling mode")
    {
        SECTION("abstract")
        {
            writer->scalingMode(cgm::ScalingMode::Abstract, 1.0f);

            const std::string str = stream.str();
            REQUIRE(str.size() == headerLen + 6);
            REQUIRE(header(str) == OpCode{PictureDescriptor, ScalingMode, 6});
            REQUIRE(i16(str, 2) == static_cast<int>(cgm::ScalingMode::Abstract));
            // TODO: decode 16.16 fixed-point as float
            // REQUIRE(f32(str, 4) == 1.0f);
        }
        SECTION("metric")
        {
            writer->scalingMode(cgm::ScalingMode::Metric, 1.0f);

            const std::string str = stream.str();
            REQUIRE(str.size() == headerLen + 6);
            REQUIRE(header(str) == OpCode{PictureDescriptor, ScalingMode, 6});
            REQUIRE(i16(str, 2) == static_cast<int>(cgm::ScalingMode::Metric));
            // TODO: decode 16.16 fixed-point as float
            //REQUIRE(f32(str, 4) == 1.0f);
        }
    }
    SECTION("color selection mode")
    {
        SECTION("indexed")
        {
            writer->colorSelectionMode(cgm::ColorMode::Indexed);

            const std::string str = stream.str();
            REQUIRE(str.size() == headerLen + 2);
            REQUIRE(header(str) == OpCode{PictureDescriptor, ColorMode, 2});
            REQUIRE(i16(str, 2) == static_cast<int>(cgm::ColorMode::Indexed));
        }
        SECTION("direct")
        {
            writer->colorSelectionMode(cgm::ColorMode::Direct);

            const std::string str = stream.str();
            REQUIRE(str.size() == headerLen + 2);
            REQUIRE(header(str) == OpCode{PictureDescriptor, ColorMode, 2});
            REQUIRE(i16(str, 2) == static_cast<int>(cgm::ColorMode::Direct));
        }
    }
    SECTION("line width specification mode")
    {
        SECTION("absolute")
        {
            writer->lineWidthSpecificationMode(cgm::SpecificationMode::Absolute);

            const std::string str = stream.str();
            REQUIRE(str.size() == headerLen + 2);
            REQUIRE(header(str) == OpCode{PictureDescriptor, LineWidthMode, 2});
            REQUIRE(i16(str, 2) == static_cast<int>(cgm::SpecificationMode::Absolute));
        }
        SECTION("scaled")
        {
            writer->lineWidthSpecificationMode(cgm::SpecificationMode::Scaled);

            const std::string str = stream.str();
            REQUIRE(str.size() == headerLen + 2);
            REQUIRE(header(str) == OpCode{PictureDescriptor, LineWidthMode, 2});
            REQUIRE(i16(str, 2) == static_cast<int>(cgm::SpecificationMode::Scaled));
        }
    }
    SECTION("marker size specification mode")
    {
        SECTION("absolute")
        {
            writer->markerSizeSpecificationMode(cgm::SpecificationMode::Absolute);

            const std::string str = stream.str();
            REQUIRE(str.size() == headerLen + 2);
            REQUIRE(header(str) == OpCode{PictureDescriptor, MarkerSizeMode, 2});
            REQUIRE(i16(str, 2) == static_cast<int>(cgm::SpecificationMode::Absolute));
        }
        SECTION("scaled")
        {
            writer->markerSizeSpecificationMode(cgm::SpecificationMode::Scaled);

            const std::string str = stream.str();
            REQUIRE(str.size() == headerLen + 2);
            REQUIRE(header(str) == OpCode{PictureDescriptor, MarkerSizeMode, 2});
            REQUIRE(i16(str, 2) == static_cast<int>(cgm::SpecificationMode::Scaled));
        }
    }
    // edge width specification mode
    SECTION("vdc extent")
    {
        writer->vdcExtent(32, 64, 640, 480);

        const std::string str = stream.str();
        REQUIRE(str.size() == headerLen + 4*2);
        REQUIRE(header(str) == OpCode{PictureDescriptor, VdcExtent, 4*2});
        REQUIRE(i16(str, 2) == 32);
        REQUIRE(i16(str, 4) == 64);
        REQUIRE(i16(str, 6) == 640);
        REQUIRE(i16(str, 8) == 480);
    }
    SECTION("background color")
    {
        writer->backgroundColor(128, 64, 32);

        const std::string str = stream.str();
        REQUIRE(str.size() == headerLen + 3 + pad1);
        REQUIRE(header(str) == OpCode{PictureDescriptor, BackgroundColor, 3});
        REQUIRE(u8(str, 2) == 128);
        REQUIRE(u8(str, 3) == 64);
        REQUIRE(u8(str, 4) == 32);
    }
    SECTION("vdc integer precision")
    {
        writer->vdcIntegerPrecisionBinary(16);

        const std::string str = stream.str();
        REQUIRE(str.size() == headerLen + 2);
        REQUIRE(header(str) == OpCode{Control, VdcIntegerPrecision, 2});
        REQUIRE(i16(str, 2) == 16);
    }
    // vdc real precision
    // auxiliary color
    // transparency
    SECTION("clip rectangle")
    {
        writer->clipRectangle(10, 20, 32, 64);

        const std::string str = stream.str();
        REQUIRE(str.size() == headerLen + 4*2);
        REQUIRE(header(str) == OpCode{Control, ClipRect, 4*2});
        REQUIRE(i16(str, 2) == 10);
        REQUIRE(i16(str, 4) == 20);
        REQUIRE(i16(str, 6) == 32);
        REQUIRE(i16(str, 8) == 64);
    }
    SECTION("clip indicator")
    {
        writer->clipIndicator(true);

        const std::string str = stream.str();
        REQUIRE(str.size() == headerLen + 2);
        REQUIRE(header(str) == OpCode{Control, ClipIndicator, 2});
        REQUIRE(i16(str, 2) == 1);
    }
    SECTION("polyline")
    {
        const std::vector<cgm::Point<int>> points{{11, 12}, {21, 22}};

        writer->polyline(points);

        const std::string str = stream.str();
        REQUIRE(str.size() == headerLen + 4*2);
        REQUIRE(header(str) == OpCode{Primitive, Polyline, 4*2});
        REQUIRE(i16(str, 2) == 11);
        REQUIRE(i16(str, 4) == 12);
        REQUIRE(i16(str, 6) == 21);
        REQUIRE(i16(str, 8) == 22);
    }
    // disjoint polyline
    SECTION("polymarker")
    {
        const std::vector<cgm::Point<int>> points{{11, 12}, {21, 22}};

        writer->polymarker(points);

        const std::string str = stream.str();
        REQUIRE(str.size() == headerLen + 4*2);
        REQUIRE(header(str) == OpCode{Primitive, Polymarker, 4*2});
        REQUIRE(i16(str, 2) == 11);
        REQUIRE(i16(str, 4) == 12);
        REQUIRE(i16(str, 6) == 21);
        REQUIRE(i16(str, 8) == 22);
    }
    SECTION("text")
    {
        const char text[]{"Hello, world!"};
        writer->text({10, 11}, cgm::TextFlag::Final, text);

        const std::string str = stream.str();
        REQUIRE(str.size() == headerLen + 3*2 + arraySize(text));
        REQUIRE(header(str) == OpCode{Primitive, Text, 3*2 + numOf(text)});
        REQUIRE(i16(str, 2) == 10);
        REQUIRE(i16(str, 4) == 11);
        REQUIRE(i16(str, 6) == static_cast<int>(cgm::TextFlag::Final));
        REQUIRE(unpack(str, 8) == text);
    }
    // restricted text
    // append text
    SECTION("polygon")
    {
        const std::vector<cgm::Point<int>> points{{10, 10}, {20, 10}, {20, 20}, {10, 20}};

        writer->polygon(points);

        const std::string str = stream.str();
        REQUIRE(str.size() == headerLen + 4*2*2);
        REQUIRE(header(str) == OpCode{Primitive, Polygon, 4*2*2});
    }
    // polygon set
    SECTION("cell array")
    {
        // clang-format off
        std::array<int, 14> cellArray{
            0, 0, 0, 0, 0, 1, 0,
            0, 0, 1, 0, 0, 0, 0};
        // clang-format on

        writer->cellArray({0, 1}, {10, 11}, {20, 21}, 8, 7, 2, cellArray.data());

        const std::string str = stream.str();
        REQUIRE(str.size() == headerLen + 2 + 11*2 + 14);
        REQUIRE(header(str) == OpCode{Primitive, CellArray, 31});
        REQUIRE(i16(str, 2) == 11*2 + 14);
        REQUIRE(i16(str, 4) == 0);
        REQUIRE(i16(str, 6) == 1);
        REQUIRE(i16(str, 8) == 10);
        REQUIRE(i16(str, 10) == 11);
        REQUIRE(i16(str, 12) == 20);
        REQUIRE(i16(str, 14) == 21);
        REQUIRE(i16(str, 16) == 7);
        REQUIRE(i16(str, 18) == 2);
        REQUIRE(i16(str, 20) == 8);
        REQUIRE(i16(str, 22) == 1);
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

        const std::string str = stream.str();
        REQUIRE(str.size() == headerLen + 2);
        REQUIRE(header(str) == OpCode{Attribute, LineType, 2});
        REQUIRE(i16(str, 2) == 1);
    }
    SECTION("line width")
    {
        writer->lineWidth(1.0f);

        const std::string str = stream.str();
        REQUIRE(str.size() == headerLen + 4);
        REQUIRE(header(str) == OpCode{Attribute, LineWidth, 4});
        // TODO: validate 16p16 width
    }
    SECTION("line color")
    {
        writer->lineColor(6);

        const std::string str = stream.str();
        REQUIRE(str.size() == headerLen + 2);
        REQUIRE(header(str) == OpCode{Attribute, LineColor, 1});
        REQUIRE(i8(str, 2) == 6);
    }
    // marker bundle index
    SECTION("marker type")
    {
        writer->markerType(6);

        const std::string str = stream.str();
        REQUIRE(str.size() == headerLen + 2);
        REQUIRE(header(str) == OpCode{Attribute, MarkerType, 2});
        REQUIRE(i16(str, 2) == 6);
    }
    SECTION("marker size")
    {
        writer->markerSize(0.1f);

        const std::string str = stream.str();
        REQUIRE(str.size() == headerLen + 4);
        REQUIRE(header(str) == OpCode{Attribute, MarkerSize, 4});
        // TODO validate 16p16 marker size
    }
    SECTION("marker color")
    {
        writer->markerColor(6);

        const std::string str = stream.str();
        REQUIRE(str.size() == headerLen + 2);
        REQUIRE(header(str) == OpCode{Attribute, MarkerColor, 1});
        REQUIRE(i8(str, 2) == 6);
    }
    // text bundle index
    SECTION("text font index")
    {
        writer->textFontIndex(6);

        const std::string str = stream.str();
        REQUIRE(str.size() == headerLen + 2);
        REQUIRE(header(str) == OpCode{Attribute, TextFontIndex, 2});
        REQUIRE(i16(str, 2) == 6);
    }
    SECTION("text precision")
    {
        SECTION("string")
        {
            writer->textPrecision(cgm::TextPrecision::String);

            const std::string str = stream.str();
            REQUIRE(str.size() == headerLen + 2);
            REQUIRE(header(str) == OpCode{Attribute, TextPrecision, 2});
            REQUIRE(i16(str, 2) == static_cast<int>(cgm::TextPrecision::String));
        }
        SECTION("character")
        {
            writer->textPrecision(cgm::TextPrecision::Character);

            const std::string str = stream.str();
            REQUIRE(str.size() == headerLen + 2);
            REQUIRE(header(str) == OpCode{Attribute, TextPrecision, 2});
            REQUIRE(i16(str, 2) == static_cast<int>(cgm::TextPrecision::Character));
        }
        SECTION("stroke")
        {
            writer->textPrecision(cgm::TextPrecision::Stroke);

            const std::string str = stream.str();
            REQUIRE(str.size() == headerLen + 2);
            REQUIRE(header(str) == OpCode{Attribute, TextPrecision, 2});
            REQUIRE(i16(str, 2) == static_cast<int>(cgm::TextPrecision::Stroke));
        }
    }
    SECTION("character expansion factor")
    {
        writer->charExpansion(0.1f);

        const std::string str = stream.str();
        REQUIRE(str.size() == headerLen + 4);
        REQUIRE(header(str) == OpCode{Attribute, CharExpansion, 4});
        // TODO: validate 16p16 char expansion factor
    }
    SECTION("character spacing")
    {
        writer->charSpacing(0.1f);

        const std::string str = stream.str();
        REQUIRE(str.size() == headerLen + 4);
        REQUIRE(header(str) == OpCode{Attribute, CharSpacing, 4});
        // TODO: validate 16p16 char spacing
    }
    SECTION("text color")
    {
        writer->textColor(6);

        const std::string str = stream.str();
        REQUIRE(str.size() == headerLen + 2);
        REQUIRE(header(str) == OpCode{Attribute, TextColor, 1});
        REQUIRE(i8(str, 2) == 6);
    }
    SECTION("character height")
    {
        writer->charHeight(12);

        const std::string str = stream.str();
        REQUIRE(str.size() == headerLen + 2);
        REQUIRE(header(str) == OpCode{Attribute, CharHeight, 2});
        REQUIRE(i16(str, 2) == 12);
    }
    SECTION("character orientation")
    {
        writer->charOrientation(0, 1, 2, 3);

        const std::string str = stream.str();
        REQUIRE(str.size() == headerLen + 4*2);
        REQUIRE(header(str) == OpCode{Attribute, CharOrientation, 4*2});
        REQUIRE(i16(str, 2) == 0);
        REQUIRE(i16(str, 4) == 1);
        REQUIRE(i16(str, 6) == 2);
        REQUIRE(i16(str, 8) == 3);
    }
    SECTION("text path")
    {
        SECTION("right")
        {
            writer->textPath(cgm::TextPath::Right);

            const std::string str = stream.str();
            REQUIRE(str.size() == headerLen + 2);
            REQUIRE(header(str) == OpCode{Attribute, TextPath, 2});
            REQUIRE(i16(str, 2) == static_cast<int>(cgm::TextPath::Right));
        }
        SECTION("left")
        {
            writer->textPath(cgm::TextPath::Left);

            const std::string str = stream.str();
            REQUIRE(str.size() == headerLen + 2);
            REQUIRE(header(str) == OpCode{Attribute, TextPath, 2});
            REQUIRE(i16(str, 2) == static_cast<int>(cgm::TextPath::Left));
        }
        SECTION("up")
        {
            writer->textPath(cgm::TextPath::Up);

            const std::string str = stream.str();
            REQUIRE(str.size() == headerLen + 2);
            REQUIRE(header(str) == OpCode{Attribute, TextPath, 2});
            REQUIRE(i16(str, 2) == static_cast<int>(cgm::TextPath::Up));
        }
        SECTION("down")
        {
            writer->textPath(cgm::TextPath::Down);

            const std::string str = stream.str();
            REQUIRE(str.size() == headerLen + 2);
            REQUIRE(header(str) == OpCode{Attribute, TextPath, 2});
            REQUIRE(i16(str, 2) == static_cast<int>(cgm::TextPath::Down));
        }
    }
    SECTION("text alignment")
    {
        SECTION("normal, normal")
        {
            writer->textAlignment(cgm::HorizAlign::Normal, cgm::VertAlign::Normal, 0.1f, 0.2f);

            const std::string str = stream.str();
            REQUIRE(str.size() == headerLen + 2*2 + 2*4);
            REQUIRE(header(str) == OpCode{Attribute, TextAlignment, 2*2 + 2*4});
            REQUIRE(i16(str, 2) == static_cast<int>(cgm::HorizAlign::Normal));
            REQUIRE(i16(str, 4) == static_cast<int>(cgm::VertAlign::Normal));
            // TODO validate 16p16 parameters
        }
        SECTION("left, top")
        {
            writer->textAlignment(cgm::HorizAlign::Left, cgm::VertAlign::Top, 0.0f, 0.0f);

            const std::string str = stream.str();
            REQUIRE(str.size() == headerLen + 2*2 + 2*4);
            REQUIRE(header(str) == OpCode{Attribute, TextAlignment, 2*2 + 2*4});
            REQUIRE(i16(str, 2) == static_cast<int>(cgm::HorizAlign::Left));
            REQUIRE(i16(str, 4) == static_cast<int>(cgm::VertAlign::Top));
            // TODO validate 16p16 parameters
        }
        SECTION("center, cap")
        {
            writer->textAlignment(cgm::HorizAlign::Center, cgm::VertAlign::Cap, 0.0f, 0.0f);

            const std::string str = stream.str();
            REQUIRE(str.size() == headerLen + 2*2 + 2*4);
            REQUIRE(header(str) == OpCode{Attribute, TextAlignment, 2*2 + 2*4});
            REQUIRE(i16(str, 2) == static_cast<int>(cgm::HorizAlign::Center));
            REQUIRE(i16(str, 4) == static_cast<int>(cgm::VertAlign::Cap));
            // TODO validate 16p16 parameters
        }
        SECTION("right, half")
        {
            writer->textAlignment(cgm::HorizAlign::Right, cgm::VertAlign::Half, 0.0f, 0.0f);

            const std::string str = stream.str();
            REQUIRE(str.size() == headerLen + 2*2 + 2*4);
            REQUIRE(header(str) == OpCode{Attribute, TextAlignment, 2*2 + 2*4});
            REQUIRE(i16(str, 2) == static_cast<int>(cgm::HorizAlign::Right));
            REQUIRE(i16(str, 4) == static_cast<int>(cgm::VertAlign::Half));
            // TODO validate 16p16 parameters
        }
        SECTION("continuous, base")
        {
            writer->textAlignment(cgm::HorizAlign::Continuous, cgm::VertAlign::Base, 0.0f, 0.0f);

            const std::string str = stream.str();
            REQUIRE(str.size() == headerLen + 2*2 + 2*4);
            REQUIRE(header(str) == OpCode{Attribute, TextAlignment, 2*2 + 2*4});
            REQUIRE(i16(str, 2) == static_cast<int>(cgm::HorizAlign::Continuous));
            REQUIRE(i16(str, 4) == static_cast<int>(cgm::VertAlign::Base));
            // TODO validate 16p16 parameters
        }
        SECTION("normal, bottom")
        {
            writer->textAlignment(cgm::HorizAlign::Normal, cgm::VertAlign::Bottom, 0.0f, 0.0f);

            const std::string str = stream.str();
            REQUIRE(str.size() == headerLen + 2*2 + 2*4);
            REQUIRE(header(str) == OpCode{Attribute, TextAlignment, 2*2 + 2*4});
            REQUIRE(i16(str, 2) == static_cast<int>(cgm::HorizAlign::Normal));
            REQUIRE(i16(str, 4) == static_cast<int>(cgm::VertAlign::Bottom));
            // TODO validate 16p16 parameters
        }
        SECTION("normal, continuous")
        {
            writer->textAlignment(cgm::HorizAlign::Normal, cgm::VertAlign::Continuous, 0.0f, 0.0f);

            const std::string str = stream.str();
            REQUIRE(str.size() == headerLen + 2*2 + 2*4);
            REQUIRE(header(str) == OpCode{Attribute, TextAlignment, 2*2 + 2*4});
            REQUIRE(i16(str, 2) == static_cast<int>(cgm::HorizAlign::Normal));
            REQUIRE(i16(str, 4) == static_cast<int>(cgm::VertAlign::Continuous));
            // TODO validate 16p16 parameters
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

            const std::string str = stream.str();
            REQUIRE(str.size() == headerLen + 2);
            REQUIRE(header(str) == OpCode{Attribute, InteriorStyle, 2});
            REQUIRE(i16(str, 2) == static_cast<int>(cgm::InteriorStyle::Hollow));
        }
        SECTION("solid")
        {
            writer->interiorStyle(cgm::InteriorStyle::Solid);

            const std::string str = stream.str();
            REQUIRE(str.size() == headerLen + 2);
            REQUIRE(header(str) == OpCode{Attribute, InteriorStyle, 2});
            REQUIRE(i16(str, 2) == static_cast<int>(cgm::InteriorStyle::Solid));
        }
        SECTION("pattern")
        {
            writer->interiorStyle(cgm::InteriorStyle::Pattern);

            const std::string str = stream.str();
            REQUIRE(str.size() == headerLen + 2);
            REQUIRE(header(str) == OpCode{Attribute, InteriorStyle, 2});
            REQUIRE(i16(str, 2) == static_cast<int>(cgm::InteriorStyle::Pattern));
        }
        SECTION("hatch")
        {
            writer->interiorStyle(cgm::InteriorStyle::Hatch);

            const std::string str = stream.str();
            REQUIRE(str.size() == headerLen + 2);
            REQUIRE(header(str) == OpCode{Attribute, InteriorStyle, 2});
            REQUIRE(i16(str, 2) == static_cast<int>(cgm::InteriorStyle::Hatch));
        }
        SECTION("empty")
        {
            writer->interiorStyle(cgm::InteriorStyle::Empty);

            const std::string str = stream.str();
            REQUIRE(header(str) == OpCode{Attribute, InteriorStyle, 2});
            REQUIRE(i16(str, 2) == static_cast<int>(cgm::InteriorStyle::Empty));
        }
    }
    SECTION("fill color")
    {
        writer->fillColor(6);

        const std::string str = stream.str();
        REQUIRE(header(str) == OpCode{Attribute, FillColor, 1});
        REQUIRE(i8(str, 2) == 6);
    }
    SECTION("hatch index")
    {
        writer->hatchIndex(6);

        const std::string str = stream.str();
        REQUIRE(header(str) == OpCode{Attribute, HatchIndex, 2});
        REQUIRE(i16(str, 2) == 6);
    }
    SECTION("pattern index")
    {
        writer->patternIndex(6);

        const std::string str = stream.str();
        REQUIRE(header(str) == OpCode{Attribute, PatternIndex, 2});
        REQUIRE(i16(str, 2) == 6);
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
        // TODO: Colors should be specified as integers with suitable representation?
        std::vector<cgm::Color> colors{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}};
        writer->colorTable(0, colors);

        const std::string str = stream.str();
        REQUIRE(str.size() == headerLen + 1 + 3*5);
        REQUIRE(header(str) == OpCode{Attribute, ColorTable, 1 + 3*5});
        REQUIRE(u8(str, 2) == 0);
        REQUIRE(u8(str, 3) == 0); REQUIRE(u8(str, 4) == 0); REQUIRE(u8(str, 5) == 0);
        REQUIRE(u8(str, 6) == 255); REQUIRE(u8(str, 7) == 0); REQUIRE(u8(str, 8) == 0);
        REQUIRE(u8(str, 9) == 0); REQUIRE(u8(str, 10) == 255); REQUIRE(u8(str, 11) == 0);
        REQUIRE(u8(str, 12) == 0); REQUIRE(u8(str, 13) == 0); REQUIRE(u8(str, 14) == 255);
        REQUIRE(u8(str, 15) == 255); REQUIRE(u8(str, 16) == 255); REQUIRE(u8(str, 17) == 255);
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
