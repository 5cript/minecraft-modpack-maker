#include <backend/fetch.hpp>
#include <backend/fetch_options.hpp>

#include <roar/curl/request.hpp>

#include <iostream>

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FetchOptions, method, headers, body)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
    FetchResponse,
    curlCode,
    status,
    proxyStatus,
    downloadSize,
    redirectUrl,
    body,
    headers)

void registerFetch(Nui::RpcHub const& hub)
{
    hub.registerFunction(
        "fetch", [&hub](std::string const& responseId, std::string const& url, FetchOptions const& options) {
            std::cout << "backend fetch req recvd\n";
            std::string body;
            auto request = Roar::Curl::Request{};

            std::unordered_map<std::string, std::string> headers;

            request.sink(body).setHeaderFields(options.headers);
            request.headerSink(headers);
            if (!options.body.empty())
                request.source(options.body);

            auto onPerform = [&body, &headers, &hub, &responseId](auto&& respone) {
                FetchResponse resp{
                    .curlCode = respone.result(),
                    .status = static_cast<int>(respone.code()),
                    .proxyStatus = static_cast<int>(respone.proxyCode()),
                    .downloadSize = static_cast<uint32_t>(respone.sizeOfDownload()),
                    .redirectUrl = respone.redirectUrl(),
                    .body = std::move(body),
                    .headers = std::move(headers)};

                std::cout << "callRemote: " << responseId << "\n";
                hub.callRemote(responseId, resp);
            };
            if (options.method == "GET")
                onPerform(request.get(url));
            else if (options.method == "POST")
                onPerform(request.post(url));
            else if (options.method == "PUT")
                onPerform(request.put(url));
            else if (options.method == "DELETE")
                onPerform(request.delete_(url));
            else if (options.method == "PATCH")
                onPerform(request.patch(url));
            else if (options.method == "HEAD")
                onPerform(request.head(url));
            else
                throw std::runtime_error("Unknown method: " + options.method);
        });
}