target_sources(minecraft-modpack-maker
    PRIVATE 
        main.cpp 
        filesystem.cpp
        fetch.cpp
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

target_include_directories(minecraft-modpack-maker PRIVATE ${CMAKE_SOURCE_DIR}/backend/include)