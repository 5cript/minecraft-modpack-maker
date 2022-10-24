#pragma once

#undef isfinite
#include <nlohmann/json.hpp>

#include <set>
#include <string>

struct Config
{
    std::set<std::string> ignoreMods = {};
    std::string updateServerIp = "";
    unsigned short updateServerPort = 25002;
    std::string loaderVersion = "";
    std::string minecraftVersion = "";
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
    Config,
    ignoreMods,
    updateServerIp,
    updateServerPort,
    loaderVersion,
    minecraftVersion)

Config loadConfig(std::filesystem::path const& selfDirectory);
void saveConfig(std::filesystem::path const& selfDirectory, Config const& config);