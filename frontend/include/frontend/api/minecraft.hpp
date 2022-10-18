#pragma once

// https://launchermeta.mojang.com/mc/game/version_manifest.json

#include <frontend/api/http.hpp>

#include <nui/frontend/utility/val_conversion.hpp>

namespace Minecraft
{
    struct Version
    {
        std::string id;
        std::string type;
        std::string url;
        std::string time;
        std::string releaseTime;
    };
    BOOST_DESCRIBE_STRUCT(Version, (), (id, type, url, time, releaseTime))

    struct Latest
    {
        std::string release;
        std::string snapshot;
    };
    BOOST_DESCRIBE_STRUCT(Latest, (), (release, snapshot))

    struct VersionQueryResponse
    {
        Latest latest;
        std::vector<Version> versions;
    };
    BOOST_DESCRIBE_STRUCT(VersionQueryResponse, (), (latest, versions))

    void getAllVersions(std::function<void(Http::Response<VersionQueryResponse> const&)> const& callback);
}