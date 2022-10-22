#include <backend/minecraft_launcher.hpp>

#include <backend/tar_extractor_sink.hpp>

#include <roar/curl/request.hpp>

#include <fstream>

MinecraftLauncher::MinecraftLauncher(Nui::RpcHub& hub)
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
}
bool MinecraftLauncher::downloadLinuxLauncher(std::filesystem::path const& whereTo)
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
bool MinecraftLauncher::downloadWindowsLauncher(std::filesystem::path const& whereTo)
{
    const auto launcherPath = whereTo / "client" / "Minecraft.exe";

    if (std::filesystem::exists(launcherPath) && std::filesystem::is_regular_file(launcherPath))
        return true;

    Roar::Curl::Request request;
    auto response = request.sink(launcherPath).followRedirects(true).get(windowsLauncherUrl);
    return response.code() == boost::beast::http::status::ok ||
        response.code() == boost::beast::http::status::no_content;
}