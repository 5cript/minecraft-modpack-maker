#pragma once

#include <frontend/api/fetch.hpp>

#include <nui/frontend/api/console.hpp>
#include <nui/frontend/api/json.hpp>
#include <nui/frontend/utility/val_conversion.hpp>

#include <emscripten/val.h>

#include <optional>

namespace Http
{
    template <typename T>
    struct Response
    {
        int code = 0;
        std::optional<T> body = std::nullopt;
    };

    template <typename T>
    void get(std::string const& url, FetchOptions options, std::function<void(Response<T> const&)> const& callback)
    {
        options.method = "GET";
        fetch(url, options, [callback = std::move(callback)](FetchResponse const& response) {
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

    template <typename T>
    Response<T> oldGet(std::string const& url, emscripten::val& options)
    {
        using namespace std::string_literals;
        options.set("method", "GET"s);
        emscripten::val response = emscripten::val::global("fetch")(url, options).await();
        Http::Response<T> result;
        result.code = response["status"].as<int>();
        if (result.code == 200)
            Nui::convertFromVal(response.call<emscripten::val>("json").await(), result.body);
        else if (result.code == 204)
            result.body = std::nullopt;
        else
            Nui::Console::error("Response is not ok", response.call<emscripten::val>("text").await());
        return result;
    }
}