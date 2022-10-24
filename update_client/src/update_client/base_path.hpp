#pragma once

#include <filesystem>

constexpr char const* modsDirName = "mods";
constexpr char const* clientDirectory = "client";

static std::filesystem::path getBasePath(std::filesystem::path const& selfDirectory)
{
#ifdef NDEBUG
    return selfDirectory;
#else
    return selfDirectory.parent_path().parent_path().parent_path().parent_path() / "dummy_dir";
    //return "/home/tim/MinecraftServers/Fabric1_17";
#endif
}