#include <backend/executeable_path.hpp>

#include <stdexcept>

// Must be last
#include <windows.h>

std::filesystem::path getExecuteablePath()
{
    char executeablePath[MAX_PATH + 1]{0};

    HMODULE hModule = GetModuleHandle(nullptr);
    if (hModule != nullptr)
    {
        // Use GetModuleFileName() with module handle to get the path
        GetModuleFileName(hModule, executeablePath, MAX_PATH);
        return std::filesystem::path{executeablePath};
    }
    throw std::runtime_error("Could not get own path");
}