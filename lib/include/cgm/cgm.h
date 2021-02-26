#pragma once

#include <stdio.h>

namespace cgm
{

enum class Encoding
{
    Binary = 7,
    ClearText = 8,
    Character = 9
};

void beginMetafile(FILE *file, Encoding enc);
void endMetafile(FILE *file);

}
