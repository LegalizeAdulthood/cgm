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
