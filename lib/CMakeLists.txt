add_library(cgm
    include/cgm/cgm.h
    include/cgm/builder.h
    impl.h
    impl.cpp
    cgm.cpp
    io.cpp
    gkscore.h
    gkscore.cpp
    gksws.h
    gksws.cpp
    ../ReadMe.md
    )
target_include_directories(cgm PUBLIC include)
target_include_directories(cgm PRIVATE .)
