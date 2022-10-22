#pragma once

#include <nui/frontend/event_system/observed_value.hpp>
#include <nui/frontend/utility/val_conversion.hpp>

#include <filesystem>

struct Mod
{
    std::string name;
    std::string id;
    std::string slug;
    std::string installedName;
    std::string installedTimestamp;
    std::string logoPng64;
    std::string newestTimestamp;
};
BOOST_DESCRIBE_STRUCT(Mod, (), (name, id, slug, installedName, installedTimestamp, logoPng64, newestTimestamp));
struct ModPack
{
    Nui::Observed<std::vector<Mod>> mods;
    std::string minecraftVersion;
    std::string modLoader;
};
BOOST_DESCRIBE_STRUCT(ModPack, (), (mods, minecraftVersion, modLoader));

class ModPackManager
{
  public:
    enum LoaderInstallStatus
    {
        NotInstalled,
        OutOfDate,
        Installed
    };

  public:
    ModPackManager();

    void open(std::filesystem::path path, std::function<void()> onOpen);
    void addMod(Mod const& mod);
    void removeMod(std::string const& id);
    void modLoader(std::string const& loader);
    void installMod(std::string const& id);
    std::string modLoader() const;
    void minecraftVersion(std::string const& version);
    std::string minecraftVersion() const;
    Nui::Observed<std::vector<Mod>> const& mods() const;
    Nui::Observed<LoaderInstallStatus> const& loaderInstallStatus() const;
    void installLoader();

  private:
    void save();
    std::filesystem::path modpackFile() const;
    void setupAndFixDirectories();
    void setupStartScripts();
    void onOpen();
    void updateLoaderInstalledStatus();
    void installLauncher();

  private:
    std::filesystem::path openPack_;
    ModPack pack_;
    Nui::Observed<LoaderInstallStatus> loaderInstallStatus_;
};