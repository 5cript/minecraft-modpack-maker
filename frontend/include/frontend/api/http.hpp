#pragma once

#include <nui/frontend/api/fetch.hpp>

#include <nui/frontend/api/console.hpp>
#include <nui/frontend/api/json.hpp>
#include <nui/frontend/utility/val_conversion.hpp>

#include <emscripten/val.h>

#include <optional>
#include <string>

namespace Http
{
    template <typename T>
    struct Response
    {
        int code = 0;
        std::optional<T> body = std::nullopt;
    };

    template <typename T>
    void get(std::string const& url, Nui::FetchOptions options, std::function<void(Response<T> const&)> const& callback)
    {
        options.method = "GET";
        fetch(url, options, [callback = std::move(callback)](Nui::FetchResponse const& response) {
            Http::Response<T> result;
            result.code = response.status;
            if (response.status == 200)
                Nui::convertFromVal(Nui::JSON::parse(response.body), result.body);
            else if (result.code == 204)
                result.body = std::nullopt;
            else
                Nui::Console::error("Response is not ok", response.body);
            callback(result);
        });
    }
}