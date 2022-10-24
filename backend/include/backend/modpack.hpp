#pragma once

#include <filesystem>
#include <nui/backend/rpc_hub.hpp>
#include <string>

class ModPack
{
  public:
    constexpr static char const* linuxLauncherUrl = "https://launcher.mojang.com/download/Minecraft.tar.gz";
    constexpr static char const* windowsLauncherUrl = "https://launcher.mojang.com/download/Minecraft.exe";

    ModPack(Nui::RpcHub& hub);

  private:
    bool downloadLinuxLauncher(std::filesystem::path const& whereTo);
    bool downloadWindowsLauncher(std::filesystem::path const& whereTo);
    bool installMod(
        std::filesystem::path const& basePath,
        std::string const& name,
        std::string const& previousName,
        std::string const& url);
    bool removeMod(std::filesystem::path const& basePath, std::string const& name);
    bool deployPack(std::filesystem::path const& packPath);
};