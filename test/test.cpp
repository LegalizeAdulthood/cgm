#include <cgm/cgm.h>

#include <catch2/catch.hpp>

#include <sstream>

TEST_CASE("clear text encoding")
{
    std::ostringstream stream;
    std::unique_ptr<cgm::MetafileWriter> writer{create(stream, cgm::Encoding::ClearText)};

    SECTION("begin metafile")
    {
        writer->beginMetafile("cgm unit test");

        REQUIRE(stream.str() == "BegMF \"cgm unit test\";\n");
    }
    SECTION("metafile end")
    {
        writer->endMetafile();

        REQUIRE(stream.str() == "EndMF;\n");
    }
    SECTION("metafile version")
    {
        writer->metafileVersion(2);

        REQUIRE(stream.str() == "MFDesc 1;\n");
    }
    SECTION("metafile description")
    {
        writer->metafileDescription("this is a foo widget");

        REQUIRE(stream.str() == "MFDesc \"this is a foo widget\";\n");
    }
    SECTION("vdc type integer")
    {
        writer->vdcType(cgm::VdcType::Integer);

        REQUIRE(stream.str() == "VDCType Integer;\n");
    }
    SECTION("vdc type real")
    {
        writer->vdcType(cgm::VdcType::Real);

        REQUIRE(stream.str() == "VDCType Real;\n");
    }
    SECTION("integer precision")
    {
        writer->intPrecisionClearText(1, 32767);

        REQUIRE(stream.str() == "IntegerPrec 1 32767;\n");
    }
    SECTION("real precision")
    {
        writer->realPrecisionClearText(-32767.f, 32767.f, 4);

        REQUIRE(stream.str() == "RealPrec -32767.000000 32767.000000 4;\n");
    }
    SECTION("index precision")
    {
        writer->indexPrecisionClearText(0, 127);

        REQUIRE(stream.str() == "IndexPrec 0 127;\n");
    }
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
}

//TEST_CASE("begin binary, end")
//{
//    std::ostringstream stream;
//    std::unique_ptr<cgm::MetafileWriter> writer{create(stream, cgm::Encoding::Binary)};
//    writer.begin();
//
//    FILE *file = fopen("Binary.cgm", "wb");
//    cgm::beginMetafile(file, cgm::Encoding::Binary);
//    cgm::endMetafile(file);
//    fclose(file);
//}
