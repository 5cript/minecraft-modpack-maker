#include <update_server/sha256.hpp>
#include <update_server/update_provider.hpp>

#include <boost/beast/http/file_body.hpp>
#include <fmt/ranges.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <set>

using json = nlohmann::json;
using namespace boost::beast::http;
using namespace std::string_literals;

namespace
{
    constexpr char const* modsDirName = "mods";
}

// #####################################################################################################################
UpdateProvider::UpdateProvider(std::filesystem::path const& serverDirectory)
    : serverDirectory_{serverDirectory}
    , localMods_{}
{
    loadLocalMods();
    minecraft_.start(serverDirectory / "server.jar");
}
//---------------------------------------------------------------------------------------------------------------------
void UpdateProvider::loadLocalMods()
{
    std::scoped_lock lock{guard_};
    try
    {
        const auto basePath = serverDirectory_;
        std::filesystem::directory_iterator mods{basePath / modsDirName}, end;

        localMods_.clear();
        for (; mods != end; ++mods)
            localMods_.push_back({.path = mods->path(), .sha256 = sha256FromFile(mods->path())});
    }
    catch (const std::exception& e)
    {
        std::cout << e.what() << '\n';
        exit(1);
    }
}
//---------------------------------------------------------------------------------------------------------------------
std::filesystem::path UpdateProvider::getModPath(std::string const& name)
{
    return serverDirectory_ / modsDirName / name;
}
//---------------------------------------------------------------------------------------------------------------------
std::filesystem::path UpdateProvider::getFilePath(std::string const& name)
{
    return serverDirectory_ / name;
}
//---------------------------------------------------------------------------------------------------------------------
void UpdateProvider::backupWorld()
{
    std::scoped_lock lock{guard_};
    for (int i = 1; i != 1000; ++i)
    {
        std::filesystem::path worldBackup{serverDirectory_ / ("world_"s + std::to_string(i))};
        if (!std::filesystem::exists(worldBackup))
        {
            std::cout << "Creating World Backup " << worldBackup.filename().string() << "\n";
            std::filesystem::copy(serverDirectory_ / "world", worldBackup, std::filesystem::copy_options::recursive);
            break;
        }
    }
}
//---------------------------------------------------------------------------------------------------------------------
bool UpdateProvider::installMods(std::string const& tarFile)
{
    // TODO: This is convenient, but with this project becoming more official, this cannot exist without password
    // checks and ssl.

    // using namespace StarTape;
    // auto inputBundle = openInputFile(tarFile);
    // auto arch = archive(inputBundle);
    // auto index = arch.makeIndex();

    // std::error_code ec;
    // std::filesystem::create_directory(serverDirectory_ / "mods_upload_temp", ec);
    // if (ec)
    // {
    //     std::cout << ec.message() << "\n";
    //     return false;
    // }
    // for (auto const& i : index)
    // {
    //     if (!i.fileName.empty() &&
    //         (i.type == Constants::TypeFlags::RegularFile || i.type == Constants::TypeFlags::RegularFileAlternate))
    //     {
    //         std::cout << "Extracting: " << serverDirectory_ / "mods_upload_temp" / i.fileName << "\n";
    //         std::ofstream writer(serverDirectory_ / "mods_upload_temp" / i.fileName,
    //         std::ios_base::binary); if (!writer.good())
    //         {
    //             std::cout << "Extraction failed, aborting!\n";
    //             return false;
    //         }
    //         index.writeFileToStream(i, writer);
    //     }
    // }
    // if (std::filesystem::exists(serverDirectory_ / "mods_backup"))
    // {
    //     std::filesystem::remove_all(serverDirectory_ / "mods_backup", ec);
    //     if (ec)
    //     {
    //         std::cout << ec.message() << "\n";
    //         return false;
    //     }
    // }
    // if (std::filesystem::exists(serverDirectory_ / "mods"))
    // {
    //     std::filesystem::rename(serverDirectory_ / "mods", serverDirectory_ / "mods_backup",
    //     ec); if (ec)
    //     {
    //         std::cout << ec.message() << "\n";
    //         return false;
    //     }
    // }
    // std::filesystem::rename(serverDirectory_ / "mods_upload_temp", serverDirectory_ / "mods",
    // ec); if (ec)
    // {
    //     std::cout << ec.message() << "\n";
    //     return false;
    // }
    // return true;
    return true;
}
//---------------------------------------------------------------------------------------------------------------------
UpdateInstructions UpdateProvider::buildDifference(std::vector<ModAndHash> const& remoteFiles)
{
    std::scoped_lock lock{guard_};

    std::set<std::string> remoteSet, localSet;
    std::transform(
        std::begin(remoteFiles),
        std::end(remoteFiles),
        std::inserter(remoteSet, std::end(remoteSet)),
        [](auto const& element) {
            return element.path.string();
        });
    std::transform(
        std::begin(localMods_),
        std::end(localMods_),
        std::inserter(localSet, std::end(localSet)),
        [](auto const& element) {
            return element.path.filename().string();
        });

    std::set<std::string> equal;
    std::vector<std::string> fresh, old;
    std::set_difference(
        std::begin(remoteSet),
        std::end(remoteSet),
        std::begin(localSet),
        std::end(localSet),
        std::inserter(old, std::end(old)));
    std::set_difference(
        std::begin(localSet),
        std::end(localSet),
        std::begin(remoteSet),
        std::end(remoteSet),
        std::inserter(fresh, std::end(fresh)));
    std::set_intersection(
        std::begin(localSet),
        std::end(localSet),
        std::begin(remoteSet),
        std::end(remoteSet),
        std::inserter(equal, std::end(equal)));

    auto hashOf = [&, this](std::string const& name) {
        auto it = std::find_if(std::begin(localMods_), std::end(localMods_), [&name](auto const& element) {
            return element.path.filename().string() == name;
        });
        if (it != std::end(localMods_))
            return it->sha256;
        return std::string{};
    };

    const auto basePath = serverDirectory_;
    for (auto const& remote : remoteFiles)
    {
        if (equal.find(remote.path.string()) != std::end(equal) && !remote.sha256.empty() &&
            hashOf(remote.path.string()) != remote.sha256)
        {
            fresh.push_back(remote.path.string());
            old.push_back(remote.path.string());
        }
    }

    fmt::print("FRESH:\n{}\n---------------\n", fresh);
    fmt::print("OUTDATED:\n{}\n---------------\n", old);

    return UpdateInstructions{.download = fresh, .remove = old};
}
//---------------------------------------------------------------------------------------------------------------------
void UpdateProvider::index(Roar::Session& session, Roar::EmptyBodyRequest&& request)
{
    std::scoped_lock lock{guard_};
    session.template send<string_body>(request)->status(status::ok).contentType("text/plain").body("Hi").commit();
}
//---------------------------------------------------------------------------------------------------------------------
void UpdateProvider::makeFileDifference(Roar::Session& session, Roar::EmptyBodyRequest&& request)
{
    std::scoped_lock lock{guard_};
    session.template read<string_body>(std::move(request))
        ->noBodyLimit()
        .commit()
        .then([this](Roar::Session& session, Roar::Request<string_body> const& req) {
            std::scoped_lock lock{guard_};
            if (req.body().empty())
            {
                session.template send<string_body>(req)
                    ->status(status::bad_request)
                    .contentType("text/plain")
                    .body("No body")
                    .commit();
                return;
            }
            auto obj = json::parse(req.body());

            std::vector<ModAndHash> files;
            for (auto const& mod : obj["mods"])
            {
                files.push_back(
                    ModAndHash{.path = mod["name"].get<std::string>(), .sha256 = mod["hash"].get<std::string>()});
            }
            auto diff = buildDifference(files);
            auto response = "{}"_json;
            response["download"] = diff.download;
            response["remove"] = diff.remove;
            session.template send<string_body>(req)
                ->status(status::ok)
                .contentType("application/json")
                .body(response.dump())
                .commit();
        });
}
//---------------------------------------------------------------------------------------------------------------------
void UpdateProvider::downloadMod(Roar::Session& session, Roar::EmptyBodyRequest&& request)
{
    std::scoped_lock lock{guard_};
    boost::beast::error_code ec;
    file_body::value_type body;
    auto const& matches = request.pathMatches();
    if (!matches || matches->size() != 2)
    {
        session.template send<string_body>(request)
            ->status(status::bad_request)
            .contentType("text/plain")
            .body("Bad Request")
            .commit();
        return;
    }
    body.open(getModPath((*matches)[1]).string().c_str(), boost::beast::file_mode::read, ec);
    if (!body.is_open())
    {
        session.template send<string_body>(request)
            ->status(status::not_found)
            .contentType("text/plain")
            .body("Not Found")
            .commit();
        return;
    }
    session.template send<file_body>(request)->status(status::ok).contentType(".jar").body(std::move(body)).commit();
}
//---------------------------------------------------------------------------------------------------------------------
void UpdateProvider::uploadMods(Roar::Session& session, Roar::EmptyBodyRequest&& request)
{
    // TODO:
    session.template send<string_body>(request)
        ->status(status::not_implemented)
        .contentType("text/plain")
        .body("Not Implemented")
        .commit();
}
//---------------------------------------------------------------------------------------------------------------------
void UpdateProvider::versions(Roar::Session& session, Roar::EmptyBodyRequest&& request)
{
    std::scoped_lock lock{guard_};
    boost::beast::error_code ec;
    file_body::value_type body;
    body.open((serverDirectory_ / "versions.json").string().c_str(), boost::beast::file_mode::read, ec);
    if (!body.is_open())
    {
        session.template send<string_body>(request)
            ->status(status::not_found)
            .contentType("text/plain")
            .body("Not Found")
            .commit();
        return;
    }
    session.template send<file_body>(request)->status(status::ok).contentType(".json").body(std::move(body)).commit();
}
// #####################################################################################################################