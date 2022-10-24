#include <backend/modpack.hpp>

#include <backend/tar_extractor_sink.hpp>

#include <roar/curl/request.hpp>

#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>

ModPack::ModPack(Nui::RpcHub& hub)
{
    hub.registerFunction("installLaunchers", [&hub, this](std::string const& responseId, std::string const& path) {
        try
        {
            if (!downloadLinuxLauncher(path))
            {
                hub.callRemote(responseId, nlohmann::json{{"success", false}, {"message", "Linux download failed."}});
                return;
            }
            if (!downloadWindowsLauncher(path))
            {
                hub.callRemote(responseId, nlohmann::json{{"success", false}, {"message", "Windows download failed."}});
                return;
            }
            hub.callRemote(
                responseId,
                nlohmann::json{
                    {"success", true},
                });
        }
        catch (std::exception const& e)
        {
            hub.callRemote(
                responseId,
                nlohmann::json{
                    {"success", false},
                    {"message", e.what()},
                });
        }
    });

    hub.registerFunction(
        "installMod",
        [&hub, this](
            std::string const& responseId,
            std::string const& basePath,
            std::string const& name,
            std::string const& previousName,
            std::string const& url) {
            try
            {
                if (!installMod(basePath, name, previousName, url))
                {
                    hub.callRemote(responseId, nlohmann::json{{"success", false}, {"message", "Mod download failed."}});
                    return;
                }
                hub.callRemote(
                    responseId,
                    nlohmann::json{
                        {"success", true},
                    });
            }
            catch (std::exception const& e)
            {
                hub.callRemote(
                    responseId,
                    nlohmann::json{
                        {"success", false},
                        {"message", e.what()},
                    });
            }
        });

    hub.registerFunction("deploy", [&hub, this](std::string const& responseId, std::string const& packPath) {
        try
        {
            if (!deployPack(packPath))
            {
                hub.callRemote(responseId, nlohmann::json{{"success", false}, {"message", "Deploy failed."}});
                return;
            }
            hub.callRemote(
                responseId,
                nlohmann::json{
                    {"success", true},
                });
        }
        catch (std::exception const& e)
        {
            hub.callRemote(
                responseId,
                nlohmann::json{
                    {"success", false},
                    {"message", e.what()},
                });
        }
    });

    hub.registerFunction(
        "removeMod",
        [&hub, this](std::string const& responseId, std::string const& packPath, std::string const& modName) {
            try
            {
                if (!removeMod(packPath, modName))
                {
                    hub.callRemote(responseId, nlohmann::json{{"success", false}, {"message", "Remove mod failed."}});
                    return;
                }
                hub.callRemote(
                    responseId,
                    nlohmann::json{
                        {"success", true},
                    });
            }
            catch (std::exception const& e)
            {
                hub.callRemote(
                    responseId,
                    nlohmann::json{
                        {"success", false},
                        {"message", e.what()},
                    });
            }
        });

    hub.registerFunction("copyExternals", [&hub, this](std::string const& responseId, std::string const& packPath) {
        try
        {
            if (!copyExternals(packPath))
            {
                hub.callRemote(responseId, nlohmann::json{{"success", false}, {"message", "Copy externals failed."}});
                return;
            }
            hub.callRemote(
                responseId,
                nlohmann::json{
                    {"success", true},
                });
        }
        catch (std::exception const& e)
        {
            hub.callRemote(
                responseId,
                nlohmann::json{
                    {"success", false},
                    {"message", e.what()},
                });
        }
    });
}
bool ModPack::copyExternals(std::filesystem::path const& packPath)
{
    std::filesystem::path externalsPath = packPath / "externals";
    if (!std::filesystem::exists(externalsPath))
        return true;

    for (auto const& entry : std::filesystem::directory_iterator(externalsPath))
    {
        std::filesystem::copy_file(
            entry.path(),
            packPath / "client" / "mods" / entry.path().filename(),
            std::filesystem::copy_options::overwrite_existing);
        std::filesystem::copy_file(
            entry.path(),
            packPath / "server" / "mods" / entry.path().filename(),
            std::filesystem::copy_options::overwrite_existing);
    }
    return true;
}
bool ModPack::removeMod(std::filesystem::path const& basePath, std::string const& name)
{
    std::filesystem::remove(basePath / "client" / "mods" / name);
    std::filesystem::remove(basePath / "server" / "mods" / name);
    return true;
}
bool ModPack::installMod(
    std::filesystem::path const& basePath,
    std::string const& name,
    std::string const& previousName,
    std::string const& url)
{
    {
        std::ofstream writer{basePath / (name + ".Mtemp"), std::ios::binary};
        auto response = Roar::Curl::Request{}.followRedirects(true).sink(writer).get(url);
        if (response.code() != boost::beast::http::status::ok)
            return false;
    }

    auto backupModFor = [&](std::string const& clientOrServer) {
        if (!previousName.empty())
        {
            const auto oldMod = basePath / clientOrServer / "mods" / previousName;
            if (std::filesystem::exists(oldMod))
            {
                const auto backupDir = basePath / clientOrServer / "mods_old";
                if (!std::filesystem::exists(backupDir))
                    std::filesystem::create_directory(backupDir);

                const auto backupPath = backupDir / oldMod.filename();
                if (std::filesystem::exists(backupPath))
                    std::filesystem::remove(backupPath);
                std::filesystem::rename(oldMod, backupPath);
            }
        }
    };

    backupModFor("client");
    backupModFor("server");

    if (std::filesystem::exists(basePath / "client" / "mods" / name))
        std::filesystem::remove(basePath / "client" / "mods" / name);
    if (std::filesystem::exists(basePath / "server" / "mods" / name))
        std::filesystem::remove(basePath / "server" / "mods" / name);

    std::filesystem::copy_file(basePath / (name + ".Mtemp"), basePath / "client" / "mods" / name);
    std::filesystem::copy_file(basePath / (name + ".Mtemp"), basePath / "server" / "mods" / name);
    std::filesystem::remove(basePath / (name + ".Mtemp"));
    return true;
}
bool ModPack::downloadLinuxLauncher(std::filesystem::path const& whereTo)
{
    const auto launcherPath = whereTo / "client" / "minecraft-launcher";

    if (std::filesystem::exists(launcherPath) && std::filesystem::is_regular_file(launcherPath))
        return true;

    {
        Roar::Curl::Request request;
        TarExtractorSink sink{std::filesystem::path{whereTo} / "client"};
        auto response = request.sink(sink).followRedirects(true).get(linuxLauncherUrl);
        sink.finalize();

        if (response.code() != boost::beast::http::status::ok &&
            response.code() != boost::beast::http::status::no_content)
            return false;
    }

    // Move launcher up:
    if (std::filesystem::exists(launcherPath))
    {
        std::filesystem::rename(launcherPath / "minecraft-launcher", whereTo / "client" / "launcher.tmp");
        std::filesystem::remove(launcherPath);
        std::filesystem::rename(whereTo / "client" / "launcher.tmp", launcherPath);
    }

    // add execute permission:
    const auto originalPerms = std::filesystem::status(launcherPath).permissions();
    std::filesystem::permissions(launcherPath, originalPerms | std::filesystem::perms::owner_exec);

    return true;
}
bool ModPack::downloadWindowsLauncher(std::filesystem::path const& whereTo)
{
    const auto launcherPath = whereTo / "client" / "Minecraft.exe";

    if (std::filesystem::exists(launcherPath) && std::filesystem::is_regular_file(launcherPath))
        return true;

    Roar::Curl::Request request;
    auto response = request.sink(launcherPath).followRedirects(true).get(windowsLauncherUrl);
    return response.code() == boost::beast::http::status::ok ||
        response.code() == boost::beast::http::status::no_content;
}
bool ModPack::deployPack(std::filesystem::path const& packPath)
{
    const auto deploymentsDir = packPath / "deployments";
    if (!std::filesystem::exists(deploymentsDir))
        std::filesystem::create_directory(deploymentsDir);

    // current date and time as string
    const auto now = std::chrono::system_clock::now();
    const auto now_c = std::chrono::system_clock::to_time_t(now);
    std::stringstream sstr;
    sstr << std::put_time(std::localtime(&now_c), "%Y-%m-%d_%H-%M-%S");

    const auto deploymentPath = deploymentsDir / sstr.str();
    std::filesystem::create_directory(deploymentPath);
    std::filesystem::create_directory(deploymentPath / "server");
    std::filesystem::create_directory(deploymentPath / "client");

    std::function<void(std::filesystem::path const& relative)> copyRecursive;
    copyRecursive = [&](std::filesystem::path const& relative) {
        const auto source = packPath / relative;
        const auto target = deploymentPath / relative;

        if (std::filesystem::is_directory(source))
        {
            std::filesystem::create_directory(target);
            for (auto const& entry : std::filesystem::directory_iterator{source})
                copyRecursive(relative / entry.path().filename());
        }
        else
        {
            std::filesystem::copy_file(source, target);
        }
    };

    const auto server = std::filesystem::path{"server"};
    const auto client = std::filesystem::path{"client"};
    const auto mcpackdev = std::filesystem::path{"mcpackdev"};
    copyRecursive("start.sh");
    copyRecursive("start.bat");
    copyRecursive("start_server.sh");
    copyRecursive("start_server.bat");
    copyRecursive(server / "mods");
    copyRecursive(server / "libraries");
    copyRecursive(server / "vanilla.jar");
    copyRecursive(server / "server.jar");
    copyRecursive(server / "versions.json");
    copyRecursive(server / "fabric-server-launcher.properties");
    copyRecursive(client / "mods");
    copyRecursive(client / "libraries");
    copyRecursive(client / "minecraft-launcher");
    copyRecursive(client / "Minecraft.exe");
    copyRecursive(client / "versions");
    copyRecursive(client / "launcher_profiles.json");
    copyRecursive(client / "shaderpacks");
    copyRecursive(client / "resourcepacks");
    copyRecursive(mcpackdev);
    return true;
}