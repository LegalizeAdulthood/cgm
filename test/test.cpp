#include <cgm/cgm.h>

#include <catch2/catch.hpp>

#include <sstream>

TEST_CASE("begin clear text, end")
{
    std::ostringstream stream;
    std::unique_ptr<cgm::MetafileWriter> writer{create(stream, cgm::Encoding::ClearText)};

    writer->beginMetafile("cgm unit test");
    writer->endMetafile();

    REQUIRE(stream.str() == "BegMF \"cgm unit test\";\nEndMF;\n");
}

TEST_CASE("metafile descriptor version")
{
    std::ostringstream stream;
    std::unique_ptr<cgm::MetafileWriter> writer{create(stream, cgm::Encoding::ClearText)};

    writer->beginMetafile("cgm unit test");
    writer->metafileVersion(2);
    writer->endMetafile();

    REQUIRE(stream.str() == "BegMF \"cgm unit test\";\n"
        "MFDesc 1;\n"
        "EndMF;\n");
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
