cmake_minimum_required(VERSION 3.21)

project(minecraft-modpack-maker VERSION 0.1.0)

add_subdirectory(dependencies/Nui)
include (${CMAKE_CURRENT_LIST_DIR}/cmake/common_options.cmake)

# Add executable (sources are added later, depending on frontend/backend)
add_executable(${PROJECT_NAME})
set_common_options(${PROJECT_NAME})

if (EMSCRIPTEN)
	# Link frontend of nui inside of emscripten
	target_link_libraries(${PROJECT_NAME} PRIVATE nui-frontend)
  
	nui_prepare_emscripten_target(
		TARGET
			${PROJECT_NAME}
		PREJS
			${CMAKE_CURRENT_SOURCE_DIR}/frontend/module.js
		EMSCRIPTEN_LINK_OPTIONS
			-sASYNCIFY=1 -sALLOW_MEMORY_GROWTH=1 # or other options, see emscripten docs.
	)
  
    add_subdirectory(frontend/src/frontend)
else()
    # Link backend of nui outside of emscripten
    target_link_libraries(${PROJECT_NAME} PRIVATE nui-backend)

    nui_set_target_output_directories(${PROJECT_NAME})

    # Creates a target that is compiled through emscripten. This target becomes the frontend part.
	nui_add_emscripten_target(${PROJECT_NAME}
		TARGET
			${PROJECT_NAME}
		PREJS 
			${CMAKE_CURRENT_SOURCE_DIR}/frontend/module.js
		SOURCE_DIR
			${CMAKE_SOURCE_DIR}
		CMAKE_OPTIONS
			-DNUI_BUILD_EXAMPLES=off
		# MAKE_OPTIONS
			# VERBOSE=1
	)

	add_subdirectory(backend/src/backend)
endif()