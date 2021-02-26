add_library(cgm
    include/cgm/cgm.h
    impl.h
    cgm.cpp
    io.cpp
    gks.h
    gkscore.h
    gkscore.cpp
    )
target_include_directories(cgm PUBLIC include)
target_include_directories(cgm PRIVATE .)
