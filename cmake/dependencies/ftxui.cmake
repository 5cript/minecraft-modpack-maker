include(FetchContent)
FetchContent_Declare(
	ftxui
	GIT_REPOSITORY https://github.com/5cript/FTXUI.git
	GIT_TAG        d188a37f134bba3f13b926a7eb7abce4e479468f    
)
FetchContent_MakeAvailable(ftxui)

set(FTXUI_BUILD_EXAMPLES off CACHE BOOL "Build ftx examples" FORCE)
set(FTXUI_BUILD_DOCS off CACHE BOOL "Build ftx tests" FORCE)