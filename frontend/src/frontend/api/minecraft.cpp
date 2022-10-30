#include <frontend/api/minecraft.hpp>

namespace Minecraft
{
    namespace
    {
        template <typename T>
        void commonGetRequest(std::string const& url, std::function<void(Http::Response<T> const&)> const& callback)
        {
            using namespace std::string_literals;
            auto fetchOptions = Nui::FetchOptions{};
            Http::get<T>(url, fetchOptions, callback);
        }
    }

    void getAllVersions(std::function<void(Http::Response<VersionQueryResponse> const&)> const& callback)
    {
        commonGetRequest<VersionQueryResponse>(
            "http://launchermeta.mojang.com/mc/game/version_manifest.json", callback);
    }
}