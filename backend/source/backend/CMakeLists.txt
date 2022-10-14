target_sources(${PROJECT_NAME} 
    PRIVATE 
        main.cpp 
)
# if windows
if(WIN32)
    target_sources(${PROJECT_NAME} 
        PRIVATE 
            executeable_path_win.cpp
    )
else()
target_sources(${PROJECT_NAME} 
    PRIVATE 
        executeable_path_nix.cpp
)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/backend/include)