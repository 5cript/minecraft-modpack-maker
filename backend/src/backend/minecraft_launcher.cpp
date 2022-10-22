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
    const auto launcherPath = whereTo / "minecraft-launcher";

    if (std::filesystem::exists(launcherPath) && std::filesystem::is_regular_file(launcherPath))
        return true;
    Roar::Curl::Request request;
    TarExtractorSink sink{std::filesystem::path{whereTo}};
    auto response = request.sink(sink).followRedirects(true).get(linuxLauncherUrl);
    sink.finalize();

    // Move launcher up:
    if (std::filesystem::exists(launcherPath))
    {
        std::filesystem::rename(launcherPath / "minecraft-launcher", whereTo / "minecraft-launcher.temp");
        std::filesystem::remove(launcherPath);
        std::filesystem::rename(whereTo / "minecraft-launcher.temp", launcherPath);
    }

    // add execute permission:
    const auto originalPerms = std::filesystem::status(launcherPath).permissions();
    std::filesystem::permissions(launcherPath, originalPerms | std::filesystem::perms::owner_exec);

    return response.code() == boost::beast::http::status::ok ||
        response.code() == boost::beast::http::status::no_content;
}
bool MinecraftLauncher::downloadWindowsLauncher(std::filesystem::path const& whereTo)
{
    if (std::filesystem::exists(whereTo / "Minecraft.exe") &&
        std::filesystem::is_regular_file(whereTo / "Minecraft.exe"))
        return true;

    Roar::Curl::Request request;
    auto response = request.sink(whereTo / "Minecraft.exe").followRedirects(true).get(windowsLauncherUrl);
    return response.code() == boost::beast::http::status::ok ||
        response.code() == boost::beast::http::status::no_content;
}