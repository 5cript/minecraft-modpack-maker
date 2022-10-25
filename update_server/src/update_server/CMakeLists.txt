add_executable(update-server
    main.cpp
    minecraft.cpp
    update_provider.cpp
)

set_target_properties(update-server PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

set(DEBUG_OPTIONS -fexceptions -g -Wall -pedantic-errors -pedantic)
set(RELEASE_OPTIONS -fexceptions -O3 -Wall -pedantic-errors -pedantic)
target_compile_options(update-server PUBLIC "$<$<CONFIG:DEBUG>:${DEBUG_OPTIONS}>")
target_compile_options(update-server PUBLIC "$<$<CONFIG:RELEASE>:${RELEASE_OPTIONS}>")

target_include_directories(update-server PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../../include)

find_package(Boost 1.80.0 REQUIRED COMPONENTS program_options filesystem system)
target_link_libraries(update-server PUBLIC roar fmt nlohmann_json Boost::program_options Boost::filesystem Boost::system)
nui_set_target_output_directories(update-server)