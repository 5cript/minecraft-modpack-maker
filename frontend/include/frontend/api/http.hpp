#pragma once

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
    Response<T> get(std::string const& url, emscripten::val& options)
    {
        using namespace std::string_literals;
        options.set("method", "GET"s);
        emscripten::val response = emscripten::val::global("fetch")(url, options).await();
        Http::Response<T> result;
        result.code = response["status"].as<int>();
        if (result.code == 200)
            Nui::convertFromVal(response.call<emscripten::val>("json").await(), result.body);
        return result;
    }
}