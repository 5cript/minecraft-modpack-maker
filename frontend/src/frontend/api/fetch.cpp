#include <frontend/api/fetch.hpp>

#include <nui/frontend/rpc_client.hpp>
#include <nui/frontend/utility/val_conversion.hpp>

void fetch(std::string const& url, FetchOptions const& options, std::function<void(FetchResponse const&)> callback)
{
    Nui::RpcClient::getRemoteCallableWithBackChannel(
        "fetch", [callback = std::move(callback)](emscripten::val response) {
            FetchResponse resp;
            Nui::convertFromVal<FetchResponse>(response[0], resp);
            callback(resp);
        })(url, options);
}