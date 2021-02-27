#include <cgm/cgm.h>

#include <catch2/catch.hpp>

#include <sstream>

TEST_CASE("metafile begin clear text")
{
    std::ostringstream stream;
    std::unique_ptr<cgm::MetafileWriter> writer{create(stream, cgm::Encoding::ClearText)};

    writer->beginMetafile("cgm unit test");

    REQUIRE(stream.str() == "BegMF \"cgm unit test\";\n");
}

TEST_CASE("metafile end clear text")
{
    std::ostringstream stream;
    std::unique_ptr<cgm::MetafileWriter> writer{create(stream, cgm::Encoding::ClearText)};

    writer->endMetafile();

    REQUIRE(stream.str() == "EndMF;\n");
}

TEST_CASE("metafile version")
{
    std::ostringstream stream;
    std::unique_ptr<cgm::MetafileWriter> writer{create(stream, cgm::Encoding::ClearText)};

    writer->metafileVersion(2);

    REQUIRE(stream.str() == "MFDesc 1;\n");
}


TEST_CASE("metafile description")
{
    std::ostringstream stream;
    std::unique_ptr<cgm::MetafileWriter> writer{create(stream, cgm::Encoding::ClearText)};

    writer->metafileDescription("this is a foo widget");

    REQUIRE(stream.str() == "MFDesc \"this is a foo widget\";\n");
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
