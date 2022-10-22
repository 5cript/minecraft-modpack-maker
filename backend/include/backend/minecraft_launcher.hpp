#pragma once

#include <filesystem>
#include <nui/backend/rpc_hub.hpp>
#include <string>

class MinecraftLauncher
{
  public:
    constexpr static char const* linuxLauncherUrl = "https://launcher.mojang.com/download/Minecraft.tar.gz";
    constexpr static char const* windowsLauncherUrl = "https://launcher.mojang.com/download/Minecraft.exe";

    MinecraftLauncher(Nui::RpcHub& hub);

  private:
    bool downloadLinuxLauncher(std::filesystem::path const& whereTo);
    bool downloadWindowsLauncher(std::filesystem::path const& whereTo);
};