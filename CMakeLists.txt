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
  
	nui_prepare_emscripten_target(
		TARGET
			minecraft-modpack-maker
		PREJS
			${CMAKE_CURRENT_SOURCE_DIR}/frontend/module.js
		EMSCRIPTEN_LINK_OPTIONS
            -sEXPORT_ALL=1
            # or other options, see emscripten docs.
			#-sALLOW_MEMORY_GROWTH=1 
	)
  
    add_subdirectory(frontend/src/frontend)
else()
    include (${CMAKE_CURRENT_LIST_DIR}/cmake/dependencies/roar.cmake)

    # Link backend of nui outside of emscripten
    target_link_libraries(minecraft-modpack-maker PRIVATE nui-backend roar)

    nui_set_target_output_directories(minecraft-modpack-maker)

    # Creates a target that is compiled through emscripten. This target becomes the frontend part.
	nui_add_emscripten_target(minecraft-modpack-maker
		TARGET
			minecraft-modpack-maker
        PARCEL_NO_OPTIMIZE
            on
		PREJS 
			${CMAKE_CURRENT_SOURCE_DIR}/frontend/module.js
		SOURCE_DIR
			${CMAKE_SOURCE_DIR}
		CMAKE_OPTIONS
			-DNUI_BUILD_EXAMPLES=off -DCMAKE_CXX_EXTENSIONS=on -DCMAKE_CXX_STANDARD=20
            -DCMAKE_BUILD_TYPE=Debug
            #"$<$<CONFIG:DEBUG>:-DCMAKE_BUILD_TYPE=Debug>"
            #"$<$<CONFIG:RELEASE>:-DCMAKE_BUILD_TYPE=Release>"
		# MAKE_OPTIONS
			# VERBOSE=1
	)

	add_subdirectory(backend/src/backend)
endif()

set_target_properties(minecraft-modpack-maker PROPERTIES CXX_EXTENSIONS ON)