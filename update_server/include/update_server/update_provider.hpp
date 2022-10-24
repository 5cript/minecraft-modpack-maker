#pragma once

#include <roar/routing/request_listener.hpp>
#include <update_server/minecraft.hpp>

#include <boost/describe/class.hpp>

#include <filesystem>
#include <string>
#include <vector>

struct UpdateFile
{
    std::filesystem::path path;
    std::string sha256;
};

struct UpdateInstructions
{
    std::vector<std::string> download;
    std::vector<std::string> remove;
};

class UpdateProvider
{
  public:
    UpdateProvider(std::filesystem::path const& serverDirectory);

  public:
    UpdateInstructions buildDifference(std::vector<UpdateFile> const& remoteFiles);
    std::filesystem::path getModPath(std::string const& name);
    std::filesystem::path getFilePath(std::string const& name);
    bool installMods(std::string const& tarFile);
    void backupWorld();

  private:
    void loadLocalMods();

  private:
    std::filesystem::path serverDirectory_;
    std::vector<std::filesystem::path> localMods_;
    Minecraft minecraft_;

  private:
    ROAR_MAKE_LISTENER(UpdateProvider);

    ROAR_GET(index)("/");
    ROAR_GET(makeFileDifference)("/make_file_difference");
    ROAR_GET(downloadMod)
    ({
        .path = "/download_mod/(.+)",
        .pathType = Roar::RoutePathType::Regex,
    });
    ROAR_POST(uploadMods)("/upload_mods");
    ROAR_GET(versions)("/versions");

  private:
    BOOST_DESCRIBE_CLASS(
        UpdateProvider,
        (),
        (),
        (),
        (roar_index, roar_makeFileDifference, roar_downloadMod, roar_uploadMods, roar_versions));
};