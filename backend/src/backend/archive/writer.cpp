#include <archive.h>
#include <backend/archive/archive.hpp>
#include <backend/archive/entry.hpp>
#include <backend/archive/error.hpp>
#include <backend/archive/writer.hpp>

#include <fstream>
#include <utility>

struct archive;

namespace Archive
{
    struct Writer::Implementation
    {
        std::unique_ptr<Archive> archive_;
        /// Used only when write to memory constructor is used.
        std::shared_ptr<std::string> outputBuffer_;

        Implementation(std::shared_ptr<std::string> outputBuffer = {})
            : archive_{std::make_unique<ArchiveWriter>()}
            , outputBuffer_{std::move(outputBuffer)}
        {}

        template <typename ReaderFunctionT>
        Error writeEntry(Entry& entry, ReaderFunctionT const& reader)
        {
            auto error = ::archive_write_header(*archive_, entry);
            if (error != ARCHIVE_OK)
            {
                return Error{error};
            }
            {
                reader([&error, this](char const* data, std::size_t size) {
                    auto bytesWritten = ::archive_write_data(*archive_, data, size);
                    if (bytesWritten == -1)
                    {
                        error = ARCHIVE_FAILED;
                        return false;
                    }
                    return true;
                });
                if (error != ARCHIVE_OK)
                {
                    return Error{error};
                }
            }
            return Error{ARCHIVE_OK};
        }
    };

    Writer::Writer(std::filesystem::path const& path)
        : impl_{std::make_unique<Writer::Implementation>()}
    {
        // Do not include PAX extensions if possible.
        ::archive_write_set_format_pax_restricted(*impl_->archive_);
        auto error = ::archive_write_open_filename(*impl_->archive_, path.c_str());
        if (error != ARCHIVE_OK)
        {
            throw Error(error, ::archive_error_string(*impl_->archive_));
        }
    }

    Writer::Writer(std::shared_ptr<std::string> outputBuffer)
        : impl_{std::make_unique<Writer::Implementation>(std::move(outputBuffer))}
    {
        // Do not include PAX extensions if possible.
        ::archive_write_set_format_pax_restricted(*impl_->archive_);

        auto error = ::archive_write_open(
            *impl_->archive_,
            this,
            // on open
            +[](::archive*, void*) {
                return ARCHIVE_OK;
            },
            // on write
            +[](::archive*, void* writer, void const* buffer, size_t length) {
                static_cast<Writer*>(writer)->impl_->outputBuffer_->append(static_cast<char const*>(buffer), length);
                return static_cast<long>(length);
            },
            // on close
            +[](::archive*, void*) {
                return ARCHIVE_OK;
            });

        if (error != ARCHIVE_OK)
        {
            throw Error(error, ::archive_error_string(*impl_->archive_));
        }
    }

    Writer::~Writer() = default;

    Error Writer::addGzipFilter()
    {
        return Error{::archive_write_add_filter_gzip(*impl_->archive_), ::archive_error_string(*impl_->archive_)};
    }

    Error Writer::addFile(std::filesystem::path const& path)
    {
        Entry entry;
        auto error = entry.setInformationFromFile(path);
        if (error)
        {
            return error;
        }
        return impl_->writeEntry(entry, [&path](auto const& feeder) {
            std::ifstream reader{path.string(), std::ios_base::binary};
            std::string buffer(copyBufferSize, '\0');
            do
            {
                reader.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
                if (!feeder(buffer.c_str(), static_cast<std::size_t>(reader.gcount())))
                {
                    break;
                }
            } while (reader.gcount() == static_cast<std::streamoff>(buffer.size()));
        });
    }

    Error Writer::addString(
        std::string const& data,
        std::filesystem::path const& pathName,
        std::filesystem::perms permissions)
    {
        Entry entry;
        entry.setPathname(pathName);
        entry.setSize(data.size());
        entry.setType(Entry::Type::RegularFile);
        entry.setPermissions(permissions);
        return impl_->writeEntry(entry, [&data](auto const& feeder) {
            feeder(data.c_str(), data.size());
        });
    }
}
