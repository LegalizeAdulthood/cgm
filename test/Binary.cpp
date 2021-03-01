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

struct OpCode
{
    int classCode;
    int opCode;
    int paramLength;
};

bool operator==(OpCode lhs, OpCode rhs)
{
    return lhs.classCode == rhs.classCode
        || lhs.opCode == rhs.opCode
        || lhs.paramLength == rhs.paramLength;
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

int i16(const std::string &str, int offset)
{
    // This is x86 endian
    const char bytes[2]{str[offset+1], str[offset]};
    return *reinterpret_cast<const std::int16_t *>(bytes);
}

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
        const OpCode op = header(str);
        REQUIRE(op.classCode == 0);
        REQUIRE(op.opCode == 1);
        REQUIRE(op.paramLength == numOf(ident));
        REQUIRE(int(data[2]) == encodedLength);
        REQUIRE(unpack(str, 2) == "cgm unit test"); 
    }
    SECTION("end metafile")
    {
        writer->endMetafile();

        const std::string str = stream.str();
        REQUIRE(header(str) == OpCode{0, 2, 0});
    }
    SECTION("begin picture")
    {
        const char ident[]{"cgm unit test"};
        writer->beginPicture(ident);

        const std::string str = stream.str();
        const int encodedLength = numOf(ident) - 1;
        REQUIRE(str.size() == std::size_t(3 + encodedLength));
        const char *data = str.data();
        REQUIRE(header(str) == OpCode{0, 3, numOf(ident)});
        REQUIRE(int(data[2]) == encodedLength);
        REQUIRE(str.substr(3) == "cgm unit test");
    }
    SECTION("begin picture body")
    {
        writer->beginPictureBody();

        const std::string str = stream.str();
        REQUIRE(header(str) == OpCode{0, 4, 0});
    }
    SECTION("end picture")
    {
        writer->endPicture();

        const std::string str = stream.str();
        REQUIRE(header(str) == OpCode{0, 5, 0});
    }
    SECTION("metafile version")
    {
        writer->metafileVersion(2);

        const std::string str = stream.str();
        // Shouldn't the param length be 2, not 1?
        const int paramLength = 1;
        REQUIRE(header(str) == OpCode{1, 1, paramLength});
        REQUIRE(i16(str, 2) == 2);
    }
    SECTION("metafile description")
    {
        const char ident[]{"this is a foo widget"};
        writer->metafileDescription(ident);

        const std::string str = stream.str();
        REQUIRE(header(str) == OpCode{1, 2, numOf(ident)});
        REQUIRE(unpack(str, 2) == ident);
    }
    SECTION("vdc type integer")
    {
        writer->vdcType(cgm::VdcType::Integer);

        const std::string str = stream.str();
        const int paramLength = 1;
        REQUIRE(header(str) == OpCode{1, 3, paramLength});
        REQUIRE(i16(str, 2) == 0);
    }
    SECTION("vdc type real")
    {
        writer->vdcType(cgm::VdcType::Real);

        const std::string str = stream.str();
        const int paramLength = 1;
        REQUIRE(header(str) == OpCode{1, 3, paramLength});
        REQUIRE(i16(str, 2) == 1);
    }
    SECTION("integer precision")
    {
        writer->intPrecisionBinary(32);

        const std::string str = stream.str();
        const int paramLength = 1;
        REQUIRE(header(str) == OpCode{1, 4, paramLength});
        REQUIRE(i16(str, 2) == 32);
    }
    SECTION("real precision")
    {
        writer->realPrecisionBinary(cgm::RealPrecision::Floating, 9, 23);

        const std::string str = stream.str();
        const int paramLength = 6;
        REQUIRE(header(str) == OpCode{1, 5, paramLength});
        REQUIRE(i16(str, 2) == 0);
        REQUIRE(i16(str, 4) == 9);
        REQUIRE(i16(str, 6) == 23);
    }
    SECTION("index precision")
    {
        writer->indexPrecisionBinary(16);

        const std::string str = stream.str();
        const int paramLength = 6;
        REQUIRE(header(str) == OpCode{1, 6, paramLength});
        REQUIRE(i16(str, 2) == 16);
    }
}

TEST_CASE("TODO", "[.]")
{
    std::ostringstream stream;
    std::unique_ptr<cgm::MetafileWriter> writer{create(stream, cgm::Encoding::Binary)};

    SECTION("color precision")
    {
        writer->colorPrecisionClearText(255);

        REQUIRE(stream.str() == "ColrPrec 255;\n");
    }
    SECTION("color index precision")
    {
        writer->colorIndexPrecisionClearText(32767);

        REQUIRE(stream.str() == "ColrIndexPrec 32767;\n");
    }
    SECTION("maximum color index")
    {
        writer->maximumColorIndex(63);

        REQUIRE(stream.str() == "MaxColrIndex 63;\n");
    }
    SECTION("color value extent")
    {
        writer->colorValueExtent(0, 63, 0, 63, 0, 63);

        REQUIRE(stream.str() == "ColrValueExt 0 0 0 63 63 63;\n");
    }
    SECTION("metafile element list")
    {
        writer->metafileElementList();

        REQUIRE(stream.str() == 
            "MFElemList \"BegMF EndMF BegPic BegPicBody EndPic MFVersion MFDesc VDCType \n"
            "   IntegerPrec RealPrec IndexPrec ColrPrec ColrIndexPrec MaxColrIndex \n"
            "   ColrValueExt MFElemList FontList CharCoding ScaleMode ColrMode \n"
            "   LineWidthMode MarkerSizeMode VDCExt BackColr VDCIntegerPrec Transparency \n"
            "   ClipRect Clip Line Marker Text Polygon CellArray LineType LineWidth \n"
            "   LineColr MarkerType MarkerSize MarkerColr TextFontIndex TextPrec CharExpan \n"
            "   CharSpace TextColr CharHeight CharOri TextPath TextAlign IntStyle FillColr \n"
            "   HatchIndex PatIndex ColrTable\";\n");
    }
    SECTION("metafile defaults replacement")
    {
        writer->metafileDefaultsReplacement();

        REQUIRE(stream.str() == "BegMFDefaults;\n"
            "EndMFDefaults;\n");
    }
    SECTION("font list")
    {
        std::vector<std::string> fonts{"Hershey Simplex", "Hershey Roman"};
        writer->fontList(fonts);

        REQUIRE(stream.str() == "FontList 'Hershey Simplex', 'Hershey Roman';\n");
    }
    // character set list
    SECTION("character coding announcer")
    {
        writer->characterCodingAnnouncer(cgm::CharCodeAnnouncer::Extended8Bit);

        REQUIRE(stream.str() == "CharCoding Extd8Bit;\n");
    }
    SECTION("scaling mode")
    {
        SECTION("abstract")
        {
            writer->scaleMode(cgm::ScaleMode::Abstract, 1.0f);

            REQUIRE(stream.str() == "ScaleMode Abstract 1.000000;\n");
        }
        SECTION("metric")
        {
            writer->scaleMode(cgm::ScaleMode::Metric, 1.0f);

            REQUIRE(stream.str() == "ScaleMode Metric 1.000000;\n");
        }
    }
    SECTION("color selection mode")
    {
        SECTION("indexed")
        {
            writer->colorMode(cgm::ColorMode::Indexed);

            REQUIRE(stream.str() == "ColrMode Indexed;\n");
        }
        SECTION("direct")
        {
            writer->colorMode(cgm::ColorMode::Direct);

            REQUIRE(stream.str() == "ColrMode Direct;\n");
        }
    }
    SECTION("line width specification mode")
    {
        SECTION("absolute")
        {
            writer->lineWidthMode(cgm::LineWidthMode::Absolute);

            REQUIRE(stream.str() == "LineWidthMode Absolute;\n");
        }
        SECTION("scaled")
        {
            writer->lineWidthMode(cgm::LineWidthMode::Scaled);

            REQUIRE(stream.str() == "LineWidthMode Scaled;\n");
        }
    }
    SECTION("marker size specification mode")
    {
        SECTION("absolute")
        {
            writer->markerSizeMode(cgm::MarkerSizeMode::Absolute);

            REQUIRE(stream.str() == "MarkerSizeMode Absolute;\n");
        }
        SECTION("scaled")
        {
            writer->markerSizeMode(cgm::MarkerSizeMode::Scaled);

            REQUIRE(stream.str() == "MarkerSizeMode Scaled;\n");
        }
    }
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
