include(${CMAKE_SOURCE_DIR}/cmake/dependencies/ftxui.cmake)

add_executable(update-client
    main.cpp
    update_client.cpp
    config.cpp
)

set_target_properties(update-client PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Compiler Options
set(DEBUG_OPTIONS -fexceptions -g -Wall -pedantic-errors -pedantic)
set(RELEASE_OPTIONS -fexceptions -O3)
target_compile_options(update-client PUBLIC "$<$<CONFIG:DEBUG>:${DEBUG_OPTIONS}>")
target_compile_options(update-client PUBLIC "$<$<CONFIG:RELEASE>:${RELEASE_OPTIONS}>")

target_include_directories(update-client PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../../../update_server/include)

find_package(Boost 1.80.0 REQUIRED COMPONENTS system filesystem)

target_link_libraries(update-client PUBLIC screen dom component roar Boost::filesystem Boost::system ssl crypto fmt)

add_custom_command(TARGET update-client POST_BUILD
  COMMAND bash -c "ldd $<TARGET_FILE:update-client>" | "grep" "clang" | awk "NF == 4 { system(\"cp \" \$3 \" $<TARGET_FILE_DIR:update-client>\") }"
  VERBATIM
)
add_custom_command(TARGET update-client POST_BUILD
  COMMAND bash -c "ldd $<TARGET_FILE:update-client>" | "grep" "mingw" | awk "NF == 4 { system(\"cp \" \$3 \" $<TARGET_FILE_DIR:update-client>\") }"
  VERBATIM
)