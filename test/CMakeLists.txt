find_package(Catch2 REQUIRED)

add_executable(test-cgm main.cpp test.cpp)
target_link_libraries(test-cgm PUBLIC cgm Catch2::Catch2)

add_custom_command(TARGET test-cgm POST_BUILD COMMAND test-cgm)
