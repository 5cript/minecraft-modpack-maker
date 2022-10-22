#pragma once

#include <filesystem>
#include <nui/backend/rpc_hub.hpp>
#include <string>

class Fabric
{
  public:
    constexpr static char const* mainPageBaseUrl = "https://maven.fabricmc.net/net/fabricmc/fabric-installer/";

    Fabric(Nui::RpcHub& hub);

  private:
    bool installFabricInstaller(std::filesystem::path const& whereTo);
    bool installFabric(std::filesystem::path const& whereTo, std::string const& mcVersion);
};