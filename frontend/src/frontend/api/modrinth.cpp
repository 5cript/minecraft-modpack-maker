#include <frontend/api/modrinth.hpp>

#include <nui/frontend/api/console.hpp>
#include <nui/frontend/api/json.hpp>

#include <emscripten/val.h>

#include <iostream>
#include <utility>

#ifndef MODMAKER_VERSION
#    define MODMAKER_VERSION "dev"
#endif

using namespace Nui;

namespace Modrinth
{
    namespace
    {
        constexpr static char const* apiBaseUrl = "https://api.modrinth.com/v2";

        // Adds user agent etc:
        emscripten::val prepareOptions()
        {
            using namespace std::string_literals;

            emscripten::val options = emscripten::val::object();
            emscripten::val headers = emscripten::val::global("Headers").new_();
            headers.call<void>(
                "append", "User-Agent"s, "github.com/5cript/minecraft-modpack-maker"s + "/" + MODMAKER_VERSION);
            options.set("headers", headers);
            return options;
        }

        std::string serializeFacets(Facets const& facets)
        {
            using namespace std::string_literals;
            std::string result = "[";

            auto serializeSingle = [&result](std::string const& name, auto const& vec) {
                if (result.size() > 1)
                    result += ",";
                result += "[";
                for (auto const& element : vec)
                    result += name + ":" + element + ",";
                result.pop_back();
                result += "]";
            };

            if (facets.versions)
                serializeSingle("versions", *facets.versions);
            if (facets.categories)
                serializeSingle("categories", *facets.categories);
            if (facets.license)
                serializeSingle("license", *facets.license);
            if (facets.project_type)
                serializeSingle("project_type", *facets.project_type);
            result += "]";
            return result;
        }

        std::string url(std::string const& path)
        {
            return std::string{apiBaseUrl} + path;
        }

        std::string assembleQuery(std::vector<std::pair<std::string, std::string>> pairs)
        {
            using namespace std::string_literals;
            std::string result = "?";
            for (auto const& [key, value] : pairs)
                result += key + "=" + value + "&";
            result.pop_back();
            return result;
        }
    }
}

namespace Modrinth::Projects
{
    Http::Response<SearchResult> search(SearchOptions const& options)
    {
        using namespace std::string_literals;

        emscripten::val fetchOptions = prepareOptions();
        fetchOptions.set("method", "GET"s);

        std::vector<std::pair<std::string, std::string>> queryParameters;
        queryParameters.emplace_back("query"s, options.query);
        std::string facets = serializeFacets(options.facets);
        if (facets != "[]")
            queryParameters.emplace_back("facets"s, facets);
        queryParameters.emplace_back("index"s, [&]() -> std::string {
            switch (options.sortBy)
            {
                case SortBy::Relevance:
                    return "relevance"s;
                case SortBy::Downloads:
                    return "downloads"s;
                case SortBy::Follows:
                    return "follows"s;
                case SortBy::Newest:
                    return "newest"s;
                case SortBy::Updated:
                    return "updated"s;
            }
        }());
        queryParameters.emplace_back("offset"s, std::to_string(options.offset));
        queryParameters.emplace_back("limit"s, std::to_string(options.limit));

        const std::string query = assembleQuery(queryParameters);
        std::cout << url("/search") + query << "\n";
        emscripten::val response = emscripten::val::global("fetch")(url("/search") + query, fetchOptions).await();

        Console::log(response);

        Http::Response<SearchResult> result;
        result.code = response["status"].as<int>();
        if (result.code == 200)
            convertFromVal(response.call<emscripten::val>("json").await(), result.body);
        return result;
    }

    template <typename T>
    Http::Response<T> commonGetRequest(std::string const& url)
    {
        using namespace std::string_literals;
        emscripten::val fetchOptions = prepareOptions();
        return Http::get<T>(url, fetchOptions);
    }

    Http::Response<Project> get(std::string const& idOrSlug)
    {
        return commonGetRequest<Project>(url("/projects/" + idOrSlug));
    }

    Http::Response<ProjectCheckStub> check(std::string const& idOrSlug)
    {
        return commonGetRequest<ProjectCheckStub>(url("/projects/" + idOrSlug + "/check"));
    }

    Http::Response<Dependencies> dependencies(std::string const& idOrSlug)
    {
        return commonGetRequest<Dependencies>(url("/projects/" + idOrSlug + "/dependencies"));
    }

    Http::Response<Version> version(std::string const& idOrSlug)
    {
        return commonGetRequest<Version>(url("/projects/" + idOrSlug + "/version"));
    }
}