target_sources(minecraft-modpack-maker
    PRIVATE 
        main.cpp 
        filesystem.cpp
        archive/error.cpp
        archive/reader.cpp
        archive/writer.cpp
        minecraft_launcher.cpp
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

target_link_libraries(minecraft-modpack-maker
    PRIVATE
        archive_static
        boost_filesystem
        boost_system
)

target_include_directories(minecraft-modpack-maker PRIVATE ${CMAKE_SOURCE_DIR}/backend/include)