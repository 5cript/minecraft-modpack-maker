target_sources(minecraft-modpack-maker
    PRIVATE 
        main.cpp 
        filesystem.cpp
        archive/error.cpp
        archive/reader.cpp
        archive/writer.cpp
        modpack.cpp
        tar_extractor_sink.cpp
        fabric.cpp
)
# if windows
if(WIN32)
    target_sources(minecraft-modpack-maker
        PRIVATE 
            executeable_path_win.cpp
    )
else()
    target_sources(minecraft-modpack-maker
        PRIVATE 
            executeable_path_nix.cpp
)
endif()

find_package(Boost 1.78.0 REQUIRED COMPONENTS filesystem system)

target_link_libraries(minecraft-modpack-maker
    PRIVATE
        archive_static
        Boost::filesystem
        Boost::system
)

target_include_directories(minecraft-modpack-maker PRIVATE ${CMAKE_SOURCE_DIR}/backend/include)