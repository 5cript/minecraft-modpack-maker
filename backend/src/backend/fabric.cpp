#include <backend/fabric.hpp>

#include <roar/curl/request.hpp>

#include <boost/process.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

namespace
{
    enum ModLoaderInstallStatus
    {
        NotInstalled,
        OutOfDate,
        Installed
    };
}

Fabric::Fabric(Nui::RpcHub& hub)
{
    hub.registerFunction(
        "fabricInstallStatus",
        [&hub](std::string const& responseId, std::string const& path, std::string const& mcVersion) {
            auto versionsPath = std::filesystem::path{path} / "client" / "versions";
            if (!std::filesystem::exists(versionsPath))
            {
                hub.callRemote(responseId, ModLoaderInstallStatus::NotInstalled);
                return;
            }
            auto it = std::filesystem::directory_iterator{versionsPath};
            auto end = std::filesystem::directory_iterator{};
            for (; it != end; ++it)
            {
                std::string filename = it->path().filename().string();
                std::string_view fileNameView = filename;

                if (fileNameView.starts_with("fabric-loader"))
                {
                    fileNameView.remove_prefix(14);
                    // split by dash
                    auto dashPos = fileNameView.find('-');
                    if (dashPos != std::string_view::npos)
                    {
                        [[maybe_unused]] std::string_view fabricVersion = fileNameView.substr(0, dashPos);
                        std::string_view minecraftVersion = fileNameView.substr(dashPos + 1);

                        // FIXME: Thge installed fabric version is relevant, the mc version is for finding the correct
                        // fabric version.
                        // if (minecraftVersion != mcVersion)
                        // {
                        //     hub.callRemote(responseId, ModLoaderInstallStatus::OutOfDate);
                        //     return;
                        // }
                        // else
                        // {
                        hub.callRemote(responseId, ModLoaderInstallStatus::Installed);
                        return;
                        // }
                    }
                    else
                    {
                        hub.callRemote(responseId, ModLoaderInstallStatus::NotInstalled);
                        return;
                    }
                }
            }

            hub.callRemote(responseId, static_cast<int>(ModLoaderInstallStatus::NotInstalled));
        });
    hub.registerFunction(
        "installFabric",
        [&hub, this](std::string const& responseId, std::string const& path, std::string const& mcVersion) {
            try
            {
                if (!installFabricInstaller(path))
                {
                    hub.callRemote(
                        responseId,
                        nlohmann::json{{"success", false}, {"message", "Fabric installer download failed."}});
                    return;
                }
                if (!installFabric(path, mcVersion))
                {
                    hub.callRemote(
                        responseId, nlohmann::json{{"success", false}, {"message", "Fabric download failed."}});
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
bool Fabric::installFabricInstaller(std::filesystem::path const& whereTo)
{
    using namespace std::string_literals;

    std::stringstream mavenXml{};
    Roar::Curl::Request{}.followRedirects(true).sink(mavenXml).verifyPeer(false).verifyHost(false).get(
        std::string{mainPageBaseUrl} + "/maven-metadata.xml");
    boost::property_tree::ptree tree;
    mavenXml.seekg(0);
    boost::property_tree::read_xml(mavenXml, tree);
    const auto latest = tree.get<std::string>("metadata.versioning.latest");
    const auto release = tree.get<std::string>("metadata.versioning.release");
    if (latest != release)
    {
        std::cout << "Warning: fabric latest != release\n";
        std::cout << "Latest: " << latest << "\n";
        std::cout << "Release: " << release << "\n";
        std::cout << "Picking latest.\n";
    }

    std::string baseName = "fabric-installer-"s + latest;
    Roar::Curl::Request{}
        .followRedirects(true)
        .verifyPeer(false)
        .verifyHost(false)
        .sink(whereTo / "mcpackdev" / "fabric-installer.jar")
        .get(std::string{mainPageBaseUrl} + "/" + latest + "/" + (baseName + ".jar"));
    Roar::Curl::Request{}
        .followRedirects(true)
        .verifyPeer(false)
        .verifyHost(false)
        .sink(whereTo / "mcpackdev" / "fabric-installer-server.jar")
        .get(std::string{mainPageBaseUrl} + "/" + latest + "/" + (baseName + "-server.jar"));
    Roar::Curl::Request{}
        .followRedirects(true)
        .verifyPeer(false)
        .verifyHost(false)
        .sink(whereTo / "mcpackdev" / "fabric-installer-windows.exe")
        .get(std::string{mainPageBaseUrl} + "/" + latest + "/" + (baseName + ".exe"));
    return true;
}
bool Fabric::installFabric(std::filesystem::path const& whereTo, std::string const& mcVersion)
{
    boost::process::child clientInstaller{
        boost::process::search_path("java"),
        "-jar",
        (whereTo / "mcpackdev" / "fabric-installer.jar").string(),
        "client",
        "-mcversion",
        mcVersion,
        "-dir",
        (whereTo / "client").string(),
    };
    clientInstaller.wait();

    boost::process::child serverInstaller{
        boost::process::search_path("java"),
        "-jar",
        (whereTo / "mcpackdev" / "fabric-installer.jar").string(),
        "server",
        "-mcversion",
        mcVersion,
        "-dir",
        (whereTo / "server").string(),
        "-downloadMinecraft",
    };
    serverInstaller.wait();

    std::filesystem::rename(whereTo / "server" / "server.jar", whereTo / "server" / "vanilla.jar");
    std::filesystem::rename(whereTo / "server" / "fabric-server-launch.jar", whereTo / "server" / "server.jar");

    {
        std::ofstream fabricServerProps{
            whereTo / "server" / "fabric-server-launcher.properties", std::ios_base::binary};
        fabricServerProps << "serverJar=vanilla.jar";
    }

    return true;
}
