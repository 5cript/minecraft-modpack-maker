#include <backend/executeable_path.hpp>

std::filesystem::path getExecuteablePath()
{
    return std::filesystem::canonical("/proc/self/exe");
}