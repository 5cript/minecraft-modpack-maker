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
    // Adds user agent etc:
    FetchOptions prepareOptions()
    {
        using namespace std::string_literals;

        FetchOptions options;
        options.headers["User-Agent"] = "github.com/5cript/minecraft-modpack-maker"s + "/" + MODMAKER_VERSION;
        return options;
    }

    namespace
    {
        constexpr static char const* apiBaseUrl = "https://api.modrinth.com/v2";

        std::string serializeFacets(Facets const& facets)
        {
            using namespace std::string_literals;
            std::string result = "[";

            auto serializeSingle = [&result](std::string const& name, auto const& vec) {
                if (result.size() > 1)
                    result += ",";
                result += "[";
                for (auto const& element : vec)
                    result += "\""s + name + ":" + element + ",";
                result.pop_back();
                result += "\"";
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

        template <typename T>
        void commonGetRequest(std::string const& url, std::function<void(Http::Response<T> const&)> const& callback)
        {
            using namespace std::string_literals;
            const auto fetchOptions = prepareOptions();
            Http::get<T>(url, fetchOptions, callback);
        }
    }
}

namespace Modrinth::Projects
{
    void search(SearchOptions const& options, std::function<void(Http::Response<SearchResult> const&)> const& callback)
    {
        using namespace std::string_literals;

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
        const auto fetchOptions = prepareOptions();
        Http::get<SearchResult>(url("/search") + query, fetchOptions, callback);
    }

    void get(std::string const& idOrSlug, std::function<void(Http::Response<Project> const&)> const& callback)
    {
        return commonGetRequest<Project>(url("/projects/" + idOrSlug), callback);
    }

    void
    check(std::string const& idOrSlug, std::function<void(Http::Response<ProjectCheckStub> const&)> const& callback)
    {
        return commonGetRequest<ProjectCheckStub>(url("/projects/" + idOrSlug + "/check"), callback);
    }

    void
    dependencies(std::string const& idOrSlug, std::function<void(Http::Response<Dependencies> const&)> const& callback)
    {
        return commonGetRequest<Dependencies>(url("/projects/" + idOrSlug + "/dependencies"), callback);
    }

    void version(std::string const& idOrSlug, std::function<void(Http::Response<Version> const&)> const& callback)
    {
        return commonGetRequest<Version>(url("/projects/" + idOrSlug + "/version"), callback);
    }
}