#pragma once

#include <frontend/api/modrinth.hpp>

#include <nui/frontend/event_system/observed_value.hpp>
#include <nui/frontend/utility/val_conversion.hpp>

#include <filesystem>

struct MinecraftVersion
{
    int major;
    int minor;
    int patch;

    static MinecraftVersion fromString(std::string_view versionView)
    {
        const auto readNumber = [](std::string_view& versionView) {
            std::string number;
            while (!versionView.empty() && versionView.front() >= '0' && versionView.front() <= '9')
            {
                number += versionView.front();
                versionView.remove_prefix(1);
            }
            return std::stol(number);
        };
        const auto major = readNumber(versionView);
        versionView.remove_prefix(1);
        const auto minor = readNumber(versionView);
        long patch = 0;
        if (!versionView.empty())
        {
            versionView.remove_prefix(1);
            patch = readNumber(versionView);
        }
        return MinecraftVersion{major, minor, patch};
    }

    bool isWithinMinor(MinecraftVersion const& other) const
    {
        return major == other.major && minor == other.minor;
    }
};

struct ModHistoryEntry
{
    std::string name;
    std::string id;
    std::string slug;
    std::string installedName;
    std::string installedTimestamp;
    std::string installedId;
};
BOOST_DESCRIBE_STRUCT(ModHistoryEntry, (), (name, id, slug, installedName, installedTimestamp, installedId));
struct Mod
{
    std::string name;
    std::string id;
    std::string slug;
    std::string installedName;
    std::string installedTimestamp;
    std::string logoPng64;
    std::string newestTimestamp;
    std::string installedId;
    std::vector<ModHistoryEntry> history;
};
BOOST_DESCRIBE_STRUCT(
    Mod,
    (),
    (name, id, slug, installedName, installedTimestamp, logoPng64, newestTimestamp, installedId, history));
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
    // fuzzy includes all minecraft versions up to the latest minor version.
    void findModVersions(
        std::string const& id,
        bool fuzzy,
        std::vector<std::string> const& allMinecraftVersions,
        bool featuredOnly,
        std::function<void(std::vector<Modrinth::Projects::Version> const& versions)> onFind);
    void installMod(Modrinth::Projects::Version const& version);
    std::string modLoader() const;
    void minecraftVersion(std::string const& version);
    std::string minecraftVersion() const;
    Nui::Observed<std::vector<Mod>> const& mods() const;
    Nui::Observed<LoaderInstallStatus> const& loaderInstallStatus() const;
    void installLoader();
    std::string loaderLowerCase() const;
    Mod const* findMod(std::string const& id);
    std::function<void()> createVersionUpdateMachine(
        std::vector<std::string> minecraftVersions,
        bool featuredOnly,
        std::function<void(bool)> onUpdateDone);
    void deploy();
    void copyExternals();
    void save();

  private:
    std::filesystem::path modpackFile() const;
    void setupAndFixDirectories();
    void setupStartScripts();
    void onOpen();
    void updateLoaderInstalledStatus();
    void installLauncher();
    void writeVersionsFile();

  private:
    std::filesystem::path openPack_;
    ModPack pack_;
    Nui::Observed<LoaderInstallStatus> loaderInstallStatus_;
};