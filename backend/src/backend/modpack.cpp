#include <backend/modpack.hpp>

#include <backend/tar_extractor_sink.hpp>

#include <roar/curl/request.hpp>

#include <fstream>

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
}
bool ModPack::installMod(
    std::filesystem::path const& basePath,
    std::string const& name,
    std::string const& previousName,
    std::string const& url)
{
    std::ofstream writer{basePath / (name + ".Mtemp"), std::ios::binary};
    auto response = Roar::Curl::Request{}.followRedirects(true).sink(writer).get(url);
    if (response.code() != boost::beast::http::status::ok)
        return false;

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