include(FetchContent)
FetchContent_Declare(
	ftxui
	GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI.git
	GIT_TAG        9bfa2416274ef9df67bce920f9d66fcf85d70b05    
)
FetchContent_MakeAvailable(ftxui)

set(FTXUI_BUILD_EXAMPLES off CACHE BOOL "Build ftx examples" FORCE)
set(FTXUI_BUILD_DOCS off CACHE BOOL "Build ftx tests" FORCE)