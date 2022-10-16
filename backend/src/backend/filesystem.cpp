#include <backend/filesystem.hpp>
#include <nui/backend/rpc_hub.hpp>

#include <fstream>

void FileSystem::registerAll(Nui::RpcHub const& hub)
{
    registerReadFile(hub);
    registerWriteFile(hub);
    registerCreateDirectory(hub);
    registerFileExists(hub);
}
void FileSystem::registerReadFile(Nui::RpcHub const& hub)
{
    hub.registerFunction("readFile", [&hub](std::string const& responseId, std::string const& path) {
        try
        {
            std::ifstream file(path, std::ios_base::binary);
            if (!file.is_open())
            {
                hub.callRemote(
                    responseId,
                    nlohmann::json{
                        {"success", false},
                        {"message", "Could not open file"},
                    });
                return;
            }
            std::string data;
            file.seekg(0, std::ios::end);
            data.resize(file.tellg());
            file.seekg(0, std::ios::beg);
            file.read(&data[0], data.size());
            hub.callRemote(
                responseId,
                nlohmann::json{
                    {"success", true},
                    {"data", std::move(data)},
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
void FileSystem::registerWriteFile(Nui::RpcHub const& hub)
{
    hub.registerFunction(
        "writeFile", [&hub](std::string const& responseId, std::string const& path, std::string const& data) {
            try
            {
                std::ofstream file(path, std::ios_base::binary);
                if (!file.is_open())
                {
                    hub.callRemote(
                        responseId,
                        nlohmann::json{
                            {"success", false},
                            {"message", "Could not open file"},
                        });
                    return;
                }
                file.write(data.data(), data.size());
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
                return;
            }
        });
}
void FileSystem::registerCreateDirectory(Nui::RpcHub const& hub)
{
    hub.registerFunction("createDirectory", [&hub](std::string const& responseId, std::string const& path) {
        try
        {
            std::filesystem::create_directory(path);
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
void FileSystem::registerFileExists(Nui::RpcHub const& hub)
{
    hub.registerFunction("fileExists", [&hub](std::string const& responseId, std::string const& path) {
        try
        {
            hub.callRemote(
                responseId,
                nlohmann::json{
                    {"success", true},
                    {"exists", std::filesystem::exists(path)},
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