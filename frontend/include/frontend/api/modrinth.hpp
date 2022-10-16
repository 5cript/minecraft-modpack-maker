#pragma once

#include <frontend/api/http.hpp>

#include <nui/frontend/api/console.hpp>
#include <nui/frontend/utility/val_conversion.hpp>

#include <boost/describe.hpp>

#include <optional>
#include <string>
#include <vector>

namespace Modrinth
{
    enum class SortBy
    {
        Relevance,
        Downloads,
        Follows,
        Newest,
        Updated
    };

    struct Facets
    {
        std::optional<std::vector<std::string>> versions = std::nullopt;
        std::optional<std::vector<std::string>> categories = std::nullopt;
        std::optional<std::vector<std::string>> license = std::nullopt;
        std::optional<std::vector<std::string>> project_type = std::nullopt;
    };

    namespace Projects
    {
        struct SearchOptions
        {
            std::string query;
            Facets facets;
            SortBy sortBy = SortBy::Relevance;
            int offset = 0;
            int limit = 10;
        };

        struct SearchHit
        {
            std::string slug;
            std::string title;
            std::string description;
            std::vector<std::string> categories;
            std::string client_side;
            std::string server_side;
            std::string project_type;
            int downloads;
            std::string icon_url;
            std::string project_id;
            std::string author;
            std::vector<std::string> display_categories;
            std::vector<std::string> versions;
            int follows;
            std::string date_created;
            std::string date_modified;
            std::string latest_version;
            std::string license;
            std::vector<std::string> gallery;
        };
        BOOST_DESCRIBE_STRUCT(
            SearchHit,
            (),
            (slug,
             title,
             description,
             categories,
             client_side,
             server_side,
             project_type,
             downloads,
             icon_url,
             project_id,
             author,
             display_categories,
             versions,
             follows,
             date_created,
             date_modified,
             latest_version,
             license,
             gallery));

        struct SearchResult
        {
            std::vector<SearchHit> hits;
            int offset;
            int limit;
            int total_hits;
        };
        BOOST_DESCRIBE_STRUCT(SearchResult, (), (hits, offset, limit, total_hits));
        Http::Response<SearchResult> search(SearchOptions const& options);

        struct License
        {
            std::string id;
            std::string name;
            std::string url;
        };
        BOOST_DESCRIBE_STRUCT(License, (), (id, name, url));

        struct Project
        {
            std::string slug;
            std::string title;
            std::string description;
            std::vector<std::string> categories;
            std::string client_side;
            std::string server_side;
            std::string body;
            std::vector<std::string> additional_categories;
            std::string issues_url;
            std::string source_url;
            std::string wiki_url;
            std::string discord_url;
            // donation_urls
            std::string project_type;
            int downloads;
            std::string icon_url;
            std::string id;
            std::string team;
            std::optional<std::string> body_url;
            std::optional<std::string> moderator_message;
            std::string published;
            std::string updated;
            std::string approved;
            int followers;
            std::string status;
            License license;
            std::vector<std::string> versions;
            // gallery.
        };
        BOOST_DESCRIBE_STRUCT(
            Project,
            (),
            (slug,
             title,
             description,
             categories,
             client_side,
             server_side,
             body,
             additional_categories,
             issues_url,
             source_url,
             wiki_url,
             discord_url,
             project_type,
             downloads,
             icon_url,
             id,
             team,
             body_url,
             moderator_message,
             published,
             updated,
             approved,
             followers,
             status,
             license,
             versions));
        Http::Response<Project> get(std::string const& idOrSlug);

        struct ProjectCheckStub
        {
            std::string id;
        };
        BOOST_DESCRIBE_STRUCT(ProjectCheckStub, (), (id));
        Http::Response<ProjectCheckStub> check(std::string const& idOrSlug);

        struct Dependency
        {
            std::string version_id;
            std::string project_id;
            std::string file_name;
            std::string dependency_type;
        };
        BOOST_DESCRIBE_STRUCT(Dependency, (), (version_id, project_id, file_name, dependency_type));
        struct File
        {
            // hashes
            std::string url;
            std::string filename;
            bool primary;
            std::size_t size;
        };
        BOOST_DESCRIBE_STRUCT(File, (), (url, filename, primary, size));
        struct Version
        {
            std::string name;
            std::string version_number;
            std::string changelog;
            std::vector<Dependency> dependencies;
            std::vector<std::string> game_versions;
            std::string version_type;
            std::vector<std::string> loaders;
            bool featured;
            std::string id;
            std::string project_id;
            std::string author_id;
            std::string date_published;
            int downloads;
            std::optional<std::string> changelog_url;
            std::vector<File> files;
        };
        BOOST_DESCRIBE_STRUCT(
            Version,
            (),
            (name,
             version_number,
             changelog,
             dependencies,
             game_versions,
             version_type,
             loaders,
             featured,
             id,
             project_id,
             author_id,
             date_published,
             downloads,
             changelog_url,
             files));
        struct Dependencies
        {
            std::vector<Project> projects;
            std::vector<Version> versions;
        };
        BOOST_DESCRIBE_STRUCT(Dependencies, (), (projects, versions));

        Http::Response<Dependencies> dependencies(std::string const& idOrSlug);
        Http::Response<Version> version(std::string const& idOrSlug);
    }
}