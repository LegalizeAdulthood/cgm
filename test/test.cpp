#include <catch2/catch.hpp>

#include <cgm/cgm.h>

#include <stdio.h>

TEST_CASE("begin clear text, end")
{
    FILE *file = fopen("ClearText.cgm", "wt");
    cgm::beginMetafile(file, cgm::Encoding::ClearText);
    cgm::endMetafile(file);
    fclose(file);
}

TEST_CASE("begin binary, end")
{
    FILE *file = fopen("Binary.cgm", "wb");
    cgm::beginMetafile(file, cgm::Encoding::Binary);
    cgm::endMetafile(file);
    fclose(file);
}
