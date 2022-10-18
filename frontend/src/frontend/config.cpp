#include <frontend/config.hpp>

#include <nui/frontend/api/console.hpp>
#include <nui/frontend/api/json.hpp>
#include <nui/frontend/rpc_client.hpp>

#include <filesystem>

using namespace Nui;

namespace
{
    static std::string packdevHome;
}

void loadConfig(std::function<void(Config&&)> onLoad)
{
    auto loadConfigImpl = [onLoad = std::move(onLoad)](std::filesystem::path const& home) {
        RpcClient::getRemoteCallableWithBackChannel("readFile", [onLoad = std::move(onLoad)](emscripten::val response) {
            if (response["success"].as<bool>())
            {
                Config cfg;
                convertFromVal<Config>(JSON::parse(response["data"]), cfg);
                onLoad(std::move(cfg));
            }
            else
                onLoad({});
        })((home / "config.json").string());
    };

    if (packdevHome.empty())
    {
        RpcClient::getRemoteCallableWithBackChannel("getPackDevHome", [loadConfigImpl](emscripten::val response) {
            packdevHome = response["path"].as<std::string>();
            loadConfigImpl(packdevHome);
        })();
    }
    else
        loadConfigImpl(packdevHome);
}
void saveConfig(Config const& config, std::function<void(bool)> onSaveComplete)
{
    auto saveConfigImpl = [&config, onSaveComplete = std::move(onSaveComplete)](std::filesystem::path const& home) {
        RpcClient::getRemoteCallableWithBackChannel(
            "writeFile", [onSaveComplete = std::move(onSaveComplete)](emscripten::val response) {
                onSaveComplete(response["success"].as<bool>());
            })((home / "config.json").string(), JSON::stringify(convertToVal(config)));
    };

    if (packdevHome.empty())
    {
        RpcClient::getRemoteCallableWithBackChannel("getPackDevHome", [saveConfigImpl](emscripten::val response) {
            packdevHome = response["path"].as<std::string>();
            saveConfigImpl(packdevHome);
        })();
    }
    else
        saveConfigImpl(packdevHome);
}