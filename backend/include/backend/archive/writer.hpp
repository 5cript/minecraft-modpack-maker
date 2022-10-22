#pragma once

#include "error.hpp"

#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>

namespace Archive
{
    class Writer
    {
      public:
        constexpr static std::size_t copyBufferSize = 4096;

      public:
        /**
         * This constructor will create the tar file in the filesystem.
         */
        Writer(std::filesystem::path const& path);
        /**
         * This constructor will create the tar file in memory (in the supplied string).
         */
        Writer(std::shared_ptr<std::string> outputBuffer);

        ~Writer();
        Error addGzipFilter();
        Error addFile(std::filesystem::path const& path);

        Error
        addString(std::string const& data, std::filesystem::path const& pathName, std::filesystem::perms permissions);

      private:
        struct Implementation;
        std::unique_ptr<Implementation> impl_;
    };
}
