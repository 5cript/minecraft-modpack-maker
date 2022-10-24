#pragma once

#include "config.hpp"
#include <update_server/sha256.hpp>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

struct HashedMod
{
    std::filesystem::path path;
    std::string name;
    std::string hash;
};

struct UpdateInstructions
{
    std::vector<std::string> download;
    std::vector<std::string> remove;
};

struct Versions
{
    std::string loaderVersion;
    std::string minecraftVersion;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(HashedMod, name, hash)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(UpdateInstructions, download, remove)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Versions, loaderVersion, minecraftVersion)

class UpdateClient
{
  public:
    struct ProgressCallbacks
    {
        std::function<bool()> onFabricInstall;
        std::function<void(int, int, std::string const&)> onDownloadProgress;
    };

  public:
    UpdateClient(std::filesystem::path selfDirectory, std::string remoteAddress, unsigned short port);
    void performUpdate(Config const& conf, ProgressCallbacks const& cbs);

  private:
    void updateMods();
    std::string url(std::string const& path) const;
    std::vector<HashedMod> loadLocalMods();
    void removeOldMods(std::vector<std::string> const& removalList);
    void downloadMods(std::vector<std::string> const& downloadList);
    void installFabric();
    std::optional<std::filesystem::path> findJava() const;

  private:
    Config conf_;
    ProgressCallbacks cbs_;
    std::filesystem::path selfDirectory_;
    std::string remoteAddress_;
    unsigned short port_;
};