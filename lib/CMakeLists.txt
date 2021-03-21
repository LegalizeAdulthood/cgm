add_library(cgm
    include/cgm/cgm.h
    include/cgm/builder.h
    binary.h
    binary.cpp
    clear_text.h
    clear_text.cpp
    impl.h
    impl.cpp
    cgm.cpp
    io.cpp
    gkscore.h
    gkscore.cpp
    gksws.h
    gksws.cpp
    stream.h
    stream.cpp
    ../ReadMe.md
    )
target_include_directories(cgm PUBLIC include)
target_include_directories(cgm PRIVATE .)
source_group("Public Headers" FILES include/cgm/cgm.h include/cgm/builder.h)
