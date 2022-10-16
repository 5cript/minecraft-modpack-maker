#include <frontend/config.hpp>

#include <nui/frontend/api/json.hpp>
#include <nui/frontend/rpc_client.hpp>

using namespace Nui;

void loadConfig(std::function<void(Config&&)> onLoad)
{
    RpcClient::getRemoteCallableWithBackChannel("readFile", [onLoad = std::move(onLoad)](emscripten::val response) {
        if (response["success"].as<bool>())
        {
            Config cfg;
            convertFromVal<Config>(JSON::parse(response["data"]), cfg);
            onLoad(std::move(cfg));
        }
        else
            onLoad({});
    })("config.json");
}
void saveConfig(Config const& config, std::function<void(bool)> onSaveComplete)
{
    RpcClient::getRemoteCallableWithBackChannel(
        "writeFile", [onSaveComplete = std::move(onSaveComplete)](emscripten::val response) {
            onSaveComplete(response["success"].as<bool>());
        })("config.json", JSON::stringify(convertToVal(config)));
}