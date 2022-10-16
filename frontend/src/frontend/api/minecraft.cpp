#include <frontend/api/minecraft.hpp>

namespace Minecraft
{
    namespace
    {
        template <typename T>
        Http::Response<T> commonGetRequest(std::string const& url)
        {
            using namespace std::string_literals;
            emscripten::val fetchOptions = emscripten::val::object();
            return Http::get<T>(url, fetchOptions);
        }
    }

    Http::Response<VersionQueryResponse> getAllVersions()
    {
        return commonGetRequest<VersionQueryResponse>("https://launchermeta.mojang.com/mc/game/version_manifest.json");
    }
}