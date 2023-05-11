#include <frontend/modpack.hpp>

#include <frontend/api/http.hpp>
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

// #####################################################################################################################
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
                pack_.mods.clear();
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
std::function<void()> ModPackManager::createVersionUpdateMachine(
    std::vector<std::string> minecraftVersions,
    bool featuredOnly,
    std::function<void(bool)> onUpdateDone)
{
    return [this,
            i = std::size_t{0},
            minecraftVersions = std::move(minecraftVersions),
            featuredOnly,
            onUpdateDone = std::move(onUpdateDone)]() mutable {
        if (i >= pack_.mods.size())
            return;

        findModVersions(
            pack_.mods.value()[i].id,
            false,
            minecraftVersions,
            featuredOnly,
            [&i, &onUpdateDone, this](std::vector<Modrinth::Projects::Version> const& versions) {
                if (!versions.empty())
                    pack_.mods[i]->newestTimestamp = versions[0].date_published;
                std::cout << "Updated: " << pack_.mods.value()[i].name << "\n";
                ++i;
                onUpdateDone(i < pack_.mods.size());
            });
    };
}
//---------------------------------------------------------------------------------------------------------------------
void ModPackManager::onOpen()
{
    std::sort(pack_.mods.value().begin(), pack_.mods.value().end(), [](Mod const& a, Mod const& b) {
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
Mod const* ModPackManager::findMod(std::string const& id)
{
    auto it = std::find_if(pack_.mods.cbegin(), pack_.mods.cend(), [&id](Mod const& mod) {
        return mod.id == id;
    });
    if (it == pack_.mods.cend())
        return nullptr;
    return &*it;
}
//---------------------------------------------------------------------------------------------------------------------
void ModPackManager::removeMod(std::string const& id)
{
    auto it = std::find_if(pack_.mods.begin(), pack_.mods.end(), [&id](Mod const& mod) {
        return mod.id == id;
    });

    if (it != pack_.mods.end())
    {
        if (!it->installedName.empty())
        {
            RpcClient::getRemoteCallableWithBackChannel("removeMod", [this, it](emscripten::val response) {
                if (response["success"].as<bool>())
                {
                    pack_.mods.erase(it);
                    save();
                }
                else
                {
                    Console::error("Failed to remove mod: ", response["message"]);
                }
            })(openPack_.string(), it->installedName);
        }
        else
        {
            pack_.mods.erase(it);
            save();
        }
    }
}
//---------------------------------------------------------------------------------------------------------------------
void ModPackManager::save()
{
    RpcClient::getRemoteCallableWithBackChannel("writeFile", [](emscripten::val response) {
        if (!response["success"].as<bool>())
            Console::error("Failed to save modpack");
    })(modpackFile().string(), JSON::stringify(convertToVal(pack_), 4));
    writeVersionsFile();
}
//---------------------------------------------------------------------------------------------------------------------
Nui::Observed<std::vector<Mod>>& ModPackManager::mods()
{
    return pack_.mods;
}
//---------------------------------------------------------------------------------------------------------------------
void ModPackManager::findModVersions(
    std::string const& id,
    bool fuzzy,
    std::vector<std::string> const& allMinecraftVersions,
    bool featuredOnly,
    std::function<void(std::vector<Modrinth::Projects::Version> const& versions)> onFind)
{
    std::vector<std::string> includedMinecraftVersions{pack_.minecraftVersion};
    if (fuzzy)
    {
        auto version = MinecraftVersion::fromString(pack_.minecraftVersion);
        for (auto const& otherVersion : allMinecraftVersions)
        {
            if (version.isWithinMinor(MinecraftVersion::fromString(otherVersion)))
                includedMinecraftVersions.push_back(otherVersion);
        }
    }

    Modrinth::Projects::version(
        id,
        loaderLowerCase(),
        includedMinecraftVersions,
        true,
        [id, onFind, featuredOnly, includedMinecraftVersions, loader = loaderLowerCase()](
            Http::Response<std::vector<Modrinth::Projects::Version>> const& versionResponse) {
            if (versionResponse.code == 200 && versionResponse.body)
            {
                if (!featuredOnly)
                {
                    Modrinth::Projects::version(
                        id,
                        loader,
                        includedMinecraftVersions,
                        false,
                        [onFind, featured = *versionResponse.body](
                            Http::Response<std::vector<Modrinth::Projects::Version>> const& versionResponse) {
                            if (versionResponse.code == 200 && versionResponse.body)
                            {
                                auto response = *versionResponse.body;
                                response.insert(response.end(), featured.begin(), featured.end());
                                std::sort(response.begin(), response.end(), [](auto const& a, auto const& b) {
                                    return emscripten::val::global("compareDates")(a.date_published, b.date_published)
                                        .template as<bool>();
                                });
                                onFind(response);
                            }
                            else
                            {
                                Console::error("Failed to get available versions.");
                            }
                        });
                }
                else
                {
                    auto response = *versionResponse.body;
                    std::sort(response.begin(), response.end(), [](auto const& a, auto const& b) {
                        return emscripten::val::global("compareDates")(a.date_published, b.date_published)
                            .template as<bool>();
                    });
                    onFind(response);
                }
            }
            else
            {
                Console::error("Failed to get available versions.");
            }
        });
}
//---------------------------------------------------------------------------------------------------------------------
std::string ModPackManager::loaderLowerCase() const
{
    std::string loader;
    std::transform(pack_.modLoader.begin(), pack_.modLoader.end(), std::back_inserter(loader), [](auto c) {
        return std::tolower(c);
    });
    return loader;
}
//---------------------------------------------------------------------------------------------------------------------
void ModPackManager::installMod(
    Modrinth::Projects::Version const& version,
    std::function<void(bool)> const& onInstallComplete)
{
    // find primary file:
    auto it = std::find_if(version.files.begin(), version.files.end(), [](auto const& file) {
        return file.primary;
    });
    if (it == std::end(version.files))
    {
        Console::error("Failed to find primary file for mod version");
        return;
    }

    // find mod in pack:
    const auto modIt = findModIterator(version.project_id);
    if (modIt == std::cend(pack_.mods))
        return;

    auto onDirCreationDone = [this, version, file = *it, mod = *modIt, onInstallComplete]() {
        RpcClient::getRemoteCallableWithBackChannel(
            "installMod", [this, version, file, mod, onInstallComplete](emscripten::val installResponse) {
                if (installResponse["success"].as<bool>())
                {
                    // refind but mutable:
                    auto modIt = std::find_if(pack_.mods.begin(), pack_.mods.end(), [&mod](auto const& m) {
                        return m->id == mod.id;
                    });
                    bumpHistory(*modIt);
                    modIt->installedName = file.filename;
                    modIt->installedTimestamp = version.date_published;
                    modIt->installedId = version.id;
                    save();
                    globalEventContext.executeActiveEventsImmediately();
                    onInstallComplete(true);
                }
                else
                {
                    Console::error("Failed to install mod", installResponse);
                    onInstallComplete(false);
                }
            })(openPack_, file.filename, mod.installedName, file.url);
    };

    // create mods directory
    RpcClient::getRemoteCallableWithBackChannel("createDirectory", [this, onDirCreationDone](emscripten::val response) {
        RpcClient::getRemoteCallableWithBackChannel("createDirectory", [onDirCreationDone](emscripten::val response) {
            onDirCreationDone();
        })(openPack_ / "server" / "mods");
    })(openPack_ / "client" / "mods");
}
//---------------------------------------------------------------------------------------------------------------------
std::vector<Mod>::const_iterator ModPackManager::findModIterator(std::string const& projectId)
{
    const auto modIt = std::find_if(pack_.mods.cbegin(), pack_.mods.cend(), [&projectId](auto const& mod) {
        return mod.id == projectId;
    });
    if (modIt == std::cend(pack_.mods))
        Console::error("Failed to find mod in pack");
    return modIt;
}
//---------------------------------------------------------------------------------------------------------------------
void ModPackManager::bumpHistory(Mod& mod)
{
    if (!mod.installedName.empty())
    {
        mod.history.push_back({
            .name = mod.name,
            .id = mod.id,
            .slug = mod.slug,
            .installedName = mod.installedName,
            .installedTimestamp = mod.installedTimestamp,
            .installedId = mod.installedId,
        });
    }
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
void ModPackManager::installLoader(std::function<void(bool)> onInstallDone)
{
    std::string remoteCallable;
    if (pack_.modLoader == "Fabric")
        remoteCallable = "installFabric";
    else if (pack_.modLoader == "Forge")
        remoteCallable = "installForge";
    else
        return;

    // TODO: show dialog
    RpcClient::getRemoteCallableWithBackChannel(
        remoteCallable, [this, onInstallDone = std::move(onInstallDone)](emscripten::val response) {
            auto success = response["success"].as<bool>();
            if (success)
                updateLoaderInstalledStatus();
            else
                Console::error("Failed to install loader", response["message"].as<std::string>());
            onInstallDone(success);
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
            Console::error("Failed to download launcher", response);
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
void ModPackManager::resetAllInstalls(std::function<void()> onResetDone)
{
    resetModsRecursive(std::begin(pack_.mods.value()), std::move(onResetDone));
}
//---------------------------------------------------------------------------------------------------------------------
void ModPackManager::resetModsRecursive(std::vector<Mod>::iterator iter, std::function<void()> onResetDone)
{
    Console::info("Resetting mod: ", iter->name);
    auto recurse = [this, onResetDone = std::move(onResetDone)](auto iter) mutable {
        ++iter;
        if (iter != std::end(pack_.mods.value()))
        {
            resetModsRecursive(iter, std::move(onResetDone));
        }
        else
        {
            save();
            {
                pack_.mods.modify();
            }
            globalEventContext.executeActiveEventsImmediately();
            onResetDone();
        }
    };

    bumpHistory(*iter);
    if (!iter->installedName.empty())
    {
        RpcClient::getRemoteCallableWithBackChannel("removeMod", [recurse, iter](emscripten::val response) mutable {
            if (response["success"].as<bool>())
            {
                iter->installedName.clear();
                iter->installedTimestamp = "";
                iter->installedId = "";
            }
            else
            {
                Console::error("Failed to remove mod: ", response["message"]);
            }
            recurse(iter);
        })(openPack_.string(), iter->installedName);
    }
    else
        recurse(iter);
}
//---------------------------------------------------------------------------------------------------------------------
void ModPackManager::installMissing(
    bool fuzzy,
    std::vector<std::string> const& allMinecraftVersions,
    bool featuredOnly,
    std::function<void(
        Mod const& mod,
        std::vector<Modrinth::Projects::Version> const& versions,
        std::function<void(std::optional<Modrinth::Projects::Version> const&)>)> onFind)
{
    installMissingRecursive(std::begin(pack_.mods.value()), fuzzy, allMinecraftVersions, featuredOnly, onFind);
}
//---------------------------------------------------------------------------------------------------------------------
void ModPackManager::installMissingRecursive(
    std::vector<Mod>::iterator iter,
    bool fuzzy,
    std::vector<std::string> const& allMinecraftVersions,
    bool featuredOnly,
    std::function<void(
        Mod const& mod,
        std::vector<Modrinth::Projects::Version> const& versions,
        std::function<void(std::optional<Modrinth::Projects::Version> const&)>)> onFind)
{
    auto doSave = [this]() {
        save();
        {
            pack_.mods.modify();
        }
        globalEventContext.executeActiveEventsImmediately();
    };

    // skip already installed mods
    for (; iter != std::end(pack_.mods.value()) && !iter->installedName.empty(); ++iter)
    {}

    if (iter == std::end(pack_.mods.value()))
    {
        doSave();
        return;
    }

    findModVersions(
        iter->id,
        fuzzy,
        allMinecraftVersions,
        featuredOnly,
        [this, iter, fuzzy, onFind, featuredOnly, &allMinecraftVersions, doSave](auto const& versions) {
            onFind(
                *iter,
                versions,
                [this, iter, fuzzy, onFind, featuredOnly, &allMinecraftVersions, doSave](auto const& maybeVersion) {
                    if (!maybeVersion)
                    {
                        Console::error("Bulk installation aborted, no version picked.");
                        doSave();
                        return;
                    }

                    installMod(
                        *maybeVersion,
                        [this, iter, fuzzy, onFind, featuredOnly, &allMinecraftVersions, doSave](bool success) mutable {
                            if (!success)
                            {
                                Console::error("Bulk installation aborted, installation failed.");
                                doSave();
                                return;
                            }
                            ++iter;
                            if (iter != std::end(pack_.mods.value()))
                                installMissingRecursive(iter, fuzzy, allMinecraftVersions, featuredOnly, onFind);
                            else
                                doSave();
                        });
                });
        });
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
        cd server
        java -jar server.jar
    )sh");
    const auto windowsServerStartScript = fixWhitespace(R"sh(
        @echo off
        cd server
        start "" "java" -jar "server.jar"
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
void ModPackManager::writeVersionsFile()
{
    RpcClient::getRemoteCallableWithBackChannel("readFile", [this](emscripten::val response) {
        if (response["success"].as<bool>())
        {
            auto data = response["data"].as<std::string>();
            auto versions = Nui::JSON::parse(data);
            versions.set("minecraftVersion", pack_.minecraftVersion);
            RpcClient::getRemoteCallableWithBackChannel("writeFile", [](emscripten::val response) {
                if (!response["success"].as<bool>())
                    Console::error("Failed to write versions file");
            })((openPack_ / "server" / "versions.json").string(), Nui::JSON::stringify(versions, 4));
        }
        else
        {
            emscripten::val versions = emscripten::val::object();
            versions.set("minecraftVersion", pack_.minecraftVersion);
            // cannot know at this point
            versions.set("loaderVersion", "");
            RpcClient::getRemoteCallableWithBackChannel("writeFile", [](emscripten::val response) {
                if (!response["success"].as<bool>())
                    Console::error("Failed to write versions file");
            })((openPack_ / "server" / "versions.json").string(), Nui::JSON::stringify(versions, 4));
        }
    })((openPack_ / "server" / "versions.json").string());
}
//---------------------------------------------------------------------------------------------------------------------
std::string ModPackManager::minecraftVersion() const
{
    return pack_.minecraftVersion;
}
//---------------------------------------------------------------------------------------------------------------------
void ModPackManager::deploy(std::function<void(bool)> onDeployDone)
{
    RpcClient::getRemoteCallableWithBackChannel(
        "deploy", [onDeployDone = std::move(onDeployDone)](emscripten::val deployResponse) {
            auto success = deployResponse["success"].as<bool>();
            if (!success)
                Console::error("Failed to deploy mod pack");
            onDeployDone(success);
        })((openPack_).string());
}
//---------------------------------------------------------------------------------------------------------------------
void ModPackManager::copyExternals(std::function<void(bool)> onCopyDone)
{
    RpcClient::getRemoteCallableWithBackChannel(
        "copyExternals", [onCopyDone = std::move(onCopyDone)](emscripten::val copyResponse) {
            auto success = copyResponse["success"].as<bool>();
            if (!success)
                Console::error("Failed to copy externals");
            onCopyDone(success);
        })((openPack_).string());
}
// #####################################################################################################################