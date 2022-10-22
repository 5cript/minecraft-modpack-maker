include(FetchContent)
FetchContent_Declare(
	archive
	GIT_REPOSITORY https://github.com/libarchive/libarchive.git
	GIT_TAG        9ff3a595aafbbbf5da4c61aa9f56a7a13daf8feb    
)
FetchContent_MakeAvailable(archive)

target_compile_options(archive PRIVATE -Wno-error=unused-function)
target_compile_options(archive_static PRIVATE -Wno-error=unused-function)