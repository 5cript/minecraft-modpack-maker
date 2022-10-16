target_sources(${PROJECT_NAME} 
    PRIVATE 
        api/http.cpp
        api/minecraft.cpp
        api/modrinth.cpp
        frontend.cpp 
        modpack.cpp
        config.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/frontend/include)