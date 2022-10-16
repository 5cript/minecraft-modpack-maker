#include <frontend/modpack.hpp>

#include <nui/frontend/api/console.hpp>
#include <nui/frontend/api/json.hpp>
#include <nui/frontend/rpc_client.hpp>
#include <nui/frontend/utility/val_conversion.hpp>

#include <algorithm>

using namespace Nui;

//#####################################################################################################################
void ModPackManager::open(std::filesystem::path path, std::function<void()> onOpen)
{
    openPack_ = std::move(path);
    RpcClient::getRemoteCallableWithBackChannel(
        "readFile", [this, onOpen = std::move(onOpen)](emscripten::val response) {
            if (response["success"].as<bool>())
            {
                convertFromVal<ModPack>(JSON::parse(response["data"]), pack_);
                onOpen();
            }
            else
            {
                RpcClient::getRemoteCallableWithBackChannel("createDirectory", [this](emscripten::val response) {
                    save();
                })((openPack_ / "mcpackdev").string());
            }
        })(modpackFile().string());

    std::sort(pack_.mods.begin(), pack_.mods.end(), [](Mod const& a, Mod const& b) {
        return a.name < b.name;
    });
}
//---------------------------------------------------------------------------------------------------------------------
void ModPackManager::addMod(Mod const& mod)
{
    auto it = std::upper_bound(pack_.mods.begin(), pack_.mods.end(), mod, [](Mod const& a, Mod const& b) {
        return a.name < b.name;
    });
    pack_.mods.insert(it, mod);
    save();
}
//---------------------------------------------------------------------------------------------------------------------
void ModPackManager::removeMod(std::string const& id)
{
    auto it = std::find_if(pack_.mods.begin(), pack_.mods.end(), [&id](Mod const& mod) {
        return mod.id == id;
    });
    if (it != pack_.mods.end())
    {
        pack_.mods.erase(it);
        save();
    }
}
//---------------------------------------------------------------------------------------------------------------------
void ModPackManager::save()
{
    RpcClient::getRemoteCallableWithBackChannel("writeFile", [](emscripten::val response) {
        if (!response["success"].as<bool>())
            Console::error("Failed to save modpack");
    })(modpackFile().string(), JSON::stringify(convertToVal(pack_)));
}
//---------------------------------------------------------------------------------------------------------------------
std::filesystem::path ModPackManager::modpackFile() const
{
    return openPack_ / "mcpackdev" / "modpack.json";
}
//---------------------------------------------------------------------------------------------------------------------
void ModPackManager::modLoader(std::string const& loader)
{
    pack_.modLoader = loader;
    save();
}
//---------------------------------------------------------------------------------------------------------------------
std::string ModPackManager::modLoader() const
{
    return pack_.modLoader;
}
//---------------------------------------------------------------------------------------------------------------------
void ModPackManager::minecraftVersion(std::string const& version)
{
    pack_.minecraftVersion = version;
    save();
}
//---------------------------------------------------------------------------------------------------------------------
std::string ModPackManager::minecraftVersion() const
{
    return pack_.minecraftVersion;
}
//#####################################################################################################################