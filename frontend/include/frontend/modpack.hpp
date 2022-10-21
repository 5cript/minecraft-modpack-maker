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
    void open(std::filesystem::path path, std::function<void()> onOpen);
    void addMod(Mod const& mod);
    void removeMod(std::string const& id);
    void modLoader(std::string const& loader);
    void installMod(std::string const& id);
    std::string modLoader() const;
    void minecraftVersion(std::string const& version);
    std::string minecraftVersion() const;
    Nui::Observed<std::vector<Mod>> const& mods() const;

  private:
    void save();
    std::filesystem::path modpackFile() const;

  private:
    std::filesystem::path openPack_;
    ModPack pack_;
};