target_sources(minecraft-modpack-maker 
    PRIVATE 
        api/http.cpp
        api/minecraft.cpp
        api/modrinth.cpp
        main.cpp 
        main_page.cpp 
        modpack.cpp
        config.cpp
        components/mod_picker.cpp
)

target_include_directories(minecraft-modpack-maker PRIVATE ${CMAKE_SOURCE_DIR}/backend/include)
target_include_directories(minecraft-modpack-maker PRIVATE ${CMAKE_SOURCE_DIR}/frontend/include)