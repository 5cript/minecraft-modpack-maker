#include <frontend/modpack.hpp>

#include <nui/frontend/api/console.hpp>
#include <nui/frontend/api/json.hpp>
#include <nui/frontend/rpc_client.hpp>
#include <nui/frontend/utility/val_conversion.hpp>

#include <algorithm>
#include <iostream>
#include <sstream>

using namespace Nui;

namespace
{
    std::string fixWhitespace(char const* literal)
    {
        std::string trimmed{literal};
        // trim front
        trimmed.erase(trimmed.begin(), std::find_if(trimmed.begin(), trimmed.end(), [](auto c) {
                          return !std::isspace(c);
                      }));
        // trim back
        trimmed.erase(
            std::find_if(
                trimmed.rbegin(),
                trimmed.rend(),
                [](auto c) {
                    return !std::isspace(c);
                })
                .base(),
            trimmed.end());
        // trim front of each line:
        std::stringstream trimmedStream{trimmed};
        std::string result;
        for (std::string line; std::getline(trimmedStream, line);)
        {
            line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](auto c) {
                           return !std::isspace(c);
                       }));
            result += line + "\n";
        }
        return result;
    }
}

//#####################################################################################################################
ModPackManager::ModPackManager()
    : openPack_{}
    , pack_{}
    , loaderInstallStatus_{LoaderInstallStatus::NotInstalled}
{}
//---------------------------------------------------------------------------------------------------------------------
void ModPackManager::open(std::filesystem::path path, std::function<void()> onOpen)
{
    openPack_ = std::move(path);
    RpcClient::getRemoteCallableWithBackChannel(
        "readFile", [this, onOpenCb = std::move(onOpen)](emscripten::val response) {
            if (response["success"].as<bool>())
            {
                convertFromVal<ModPack>(JSON::parse(response["data"]), pack_);
                this->onOpen();
                onOpenCb();
            }
            else
            {
                RpcClient::getRemoteCallableWithBackChannel("createDirectory", [this](emscripten::val response) {
                    save();
                })((openPack_ / "mcpackdev").string());
            }
        })(modpackFile().string());
}
//---------------------------------------------------------------------------------------------------------------------
void ModPackManager::onOpen()
{
    std::sort(pack_.mods.begin(), pack_.mods.end(), [](Mod const& a, Mod const& b) {
        return a.name < b.name;
    });
    // transform loader to lower
    std::transform(pack_.modLoader.begin(), pack_.modLoader.end(), pack_.modLoader.begin(), [](auto c) {
        return std::tolower(c);
    });
    pack_.modLoader[0] = std::toupper(pack_.modLoader[0]);

    setupAndFixDirectories();
    updateLoaderInstalledStatus();
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
    })(modpackFile().string(), JSON::stringify(convertToVal(pack_), 4));
}
//---------------------------------------------------------------------------------------------------------------------
Nui::Observed<std::vector<Mod>> const& ModPackManager::mods() const
{
    return pack_.mods;
}
//---------------------------------------------------------------------------------------------------------------------
void ModPackManager::installMod(std::string const& id)
{
    std::cout << "TODO - install: " << id << "\n";
}
//---------------------------------------------------------------------------------------------------------------------
Nui::Observed<ModPackManager::LoaderInstallStatus> const& ModPackManager::loaderInstallStatus() const
{
    return loaderInstallStatus_;
}
//---------------------------------------------------------------------------------------------------------------------
void ModPackManager::setupAndFixDirectories()
{
    RpcClient::getRemoteCallableWithBackChannel("createDirectory", [this](emscripten::val) {
        setupStartScripts();
        installLauncher();
    })((openPack_ / "client").string());
    RpcClient::getRemoteCallableWithBackChannel("createDirectory", [](emscripten::val) {})(
        (openPack_ / "server").string());
}
//---------------------------------------------------------------------------------------------------------------------
void ModPackManager::installLoader()
{
    std::string remoteCallable;
    if (pack_.modLoader == "Fabric")
        remoteCallable = "installFabric";
    else if (pack_.modLoader == "Forge")
        remoteCallable = "installForge";
    else
        return;

    // TODO: show dialog
    RpcClient::getRemoteCallableWithBackChannel(remoteCallable, [this](emscripten::val response) {
        if (response["success"].as<bool>())
        {
            updateLoaderInstalledStatus();
        }
        else
        {
            Console::error("Failed to install loader");
        }
    })((openPack_).string(), pack_.minecraftVersion);
}
//---------------------------------------------------------------------------------------------------------------------
void ModPackManager::installLauncher()
{
    RpcClient::getRemoteCallableWithBackChannel("installLaunchers", [](emscripten::val response) {
        if (response["success"].as<bool>())
        {
            // TODO: show dialog
        }
        else
        {
            Console::error("Failed to download launcher");
        }
    })((openPack_).string());
}
//---------------------------------------------------------------------------------------------------------------------
void ModPackManager::updateLoaderInstalledStatus()
{
    if (pack_.modLoader == "Fabric")
    {
        RpcClient::getRemoteCallableWithBackChannel("fabricInstallStatus", [this](emscripten::val response) {
            loaderInstallStatus_ = static_cast<LoaderInstallStatus>(response.as<int>());
            globalEventContext.executeActiveEventsImmediately();
        })((openPack_).string(), pack_.minecraftVersion);
    }
}
//---------------------------------------------------------------------------------------------------------------------
void ModPackManager::setupStartScripts()
{
    const auto linuxClientStartScript = fixWhitespace(R"sh(
        #!/bin/bash
        WORKDIR="$(dirname "$(readlink -f "$0")")/client"
        ./client/minecraft-launcher --workDir $WORKDIR
    )sh");
    const auto windowsClientStartScript = fixWhitespace(R"bat(
        @echo off
        set WORKDIR=%cd%\client
        start "" "client/Minecraft.exe" --workDir "%WORKDIR%"
    )bat");
    const auto linuxServerStartScript = fixWhitespace(R"sh(
        #!/bin/bash
        java -jar server/server.jar
    )sh");
    const auto windowsServerStartScript = fixWhitespace(R"sh(
        @echo off
        start "" "java" -jar "server/server.jar"
    )sh");

    RpcClient::getRemoteCallableWithBackChannel("writeFile", [](emscripten::val) {})(
        (openPack_ / "start.sh").string(), linuxClientStartScript);
    RpcClient::getRemoteCallableWithBackChannel("writeFile", [](emscripten::val) {})(
        (openPack_ / "start.bat").string(), windowsClientStartScript);
    RpcClient::getRemoteCallableWithBackChannel("writeFile", [](emscripten::val) {})(
        (openPack_ / "start_server.sh").string(), linuxServerStartScript);
    RpcClient::getRemoteCallableWithBackChannel("writeFile", [](emscripten::val) {})(
        (openPack_ / "start_server.bat").string(), windowsServerStartScript);
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