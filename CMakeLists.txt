cmake_minimum_required(VERSION 3.21)

project(minecraft-modpack-maker VERSION 0.1.0)

add_subdirectory(dependencies/Nui)
include (${CMAKE_CURRENT_LIST_DIR}/cmake/common_options.cmake)

# Add executable (sources are added later, depending on frontend/backend)
add_executable(minecraft-modpack-maker)
set_common_options(minecraft-modpack-maker)

set(ROAR_ENABLE_NLOHMANN_JSON OFF)

if (EMSCRIPTEN)
	# Link frontend of nui inside of emscripten
	target_link_libraries(minecraft-modpack-maker PRIVATE nui-frontend)

    target_compile_definitions(minecraft-modpack-maker PUBLIC NUI_VAL_CONVERT_ASSERTIONS=1) 
  
	nui_prepare_emscripten_target(
		TARGET
			minecraft-modpack-maker
        PARCEL_ARGS
            --no-optimize
		PREJS
			${CMAKE_CURRENT_SOURCE_DIR}/frontend/src/frontend/js/module.js
		EMSCRIPTEN_LINK_OPTIONS
            -sEXPORTED_FUNCTIONS=_frontendMain
            -O2
            # -g
            # or other options, see emscripten docs.
			#-sALLOW_MEMORY_GROWTH=1 
        EMSCRIPTEN_COMPILE_OPTIONS
            -O3
            # -g
	)
  
    add_subdirectory(frontend/src/frontend)
else()
    # Link backend of nui outside of emscripten
    target_link_libraries(minecraft-modpack-maker PRIVATE nui-backend roar)

    include(cmake/dependencies/libarchive.cmake)

    nui_set_target_output_directories(minecraft-modpack-maker)

    # Creates a target that is compiled through emscripten. This target becomes the frontend part.
	nui_add_emscripten_target(minecraft-modpack-maker
		TARGET
			minecraft-modpack-maker
        PREJS
            ${CMAKE_CURRENT_SOURCE_DIR}/frontend/src/frontend/js/module.js
		SOURCE_DIR
			${CMAKE_SOURCE_DIR}
		CMAKE_OPTIONS
			-DNUI_BUILD_EXAMPLES=off -DCMAKE_CXX_EXTENSIONS=on -DCMAKE_CXX_STANDARD=20
            -DCMAKE_BUILD_TYPE=Release
            #-DCMAKE_BUILD_TYPE=Debug
            #"$<$<CONFIG:DEBUG>:-DCMAKE_BUILD_TYPE=Debug>"
            #"$<$<CONFIG:RELEASE>:-DCMAKE_BUILD_TYPE=Release>"
		MAKE_OPTIONS
			#VERBOSE=1
	)

	add_subdirectory(backend/src/backend)
    add_subdirectory(update_server/src/update_server)
    add_subdirectory(update_client/src/update_client)
endif()

set_target_properties(minecraft-modpack-maker PROPERTIES CXX_EXTENSIONS ON)