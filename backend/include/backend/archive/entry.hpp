#pragma once

#include "error.hpp"

// libarchive
#include <archive.h>
#include <archive_entry.h>

#include <filesystem>
#include <type_traits>

namespace Archive
{
    class Entry
    {
      public:
        enum Type : unsigned int
        {
            RegularFile = AE_IFREG,
            SymbolicLink = AE_IFLNK,
            Socket = AE_IFSOCK,
            CharacterDevice = AE_IFCHR,
            BlockDevice = AE_IFBLK,
            Directory = AE_IFDIR,
            Pipe = AE_IFIFO
        };

        explicit Entry(::archive_entry* entry)
            : entry_{entry}
        {}
        Entry()
            : entry_{::archive_entry_new()}
        {}
        ~Entry()
        {
            if (entry_ != nullptr)
                ::archive_entry_free(entry_);
        }

        ::archive_entry* release()
        {
            auto cpy = entry_;
            entry_ = nullptr;
            return cpy;
        }

        operator std::add_pointer_t<::archive_entry>()
        {
            return entry_;
        }
        operator std::add_pointer_t<::archive_entry const>() const
        {
            return entry_;
        }

        std::filesystem::path getPathname() const
        {
            return ::archive_entry_pathname(entry_);
        }
        uid_t getUid() const
        {
            return static_cast<uid_t>(::archive_entry_uid(entry_));
        }
        gid_t getGid() const
        {
            return static_cast<gid_t>(::archive_entry_gid(entry_));
        }
        void setPathname(std::filesystem::path const& path)
        {
            ::archive_entry_set_pathname(entry_, path.c_str());
        }
        std::size_t getSize() const
        {
            return static_cast<std::size_t>(::archive_entry_size(entry_));
        }
        void setSize(std::size_t size)
        {
            ::archive_entry_set_size(entry_, static_cast<la_int64_t>(size));
        }
        void setType(Type type)
        {
            return ::archive_entry_set_filetype(entry_, static_cast<unsigned int>(type));
        }
        Type getType() const
        {
            return static_cast<Type>(::archive_entry_filetype(entry_));
        }
        /**
         *  @param permissions a classic unix permission code, like 644, 777 etc
         */
        void setPermissions(std::filesystem::perms permissions)
        {
            ::archive_entry_set_perm(entry_, static_cast<mode_t>(permissions));
        }
        std::filesystem::perms getPermissions() const
        {
            return static_cast<std::filesystem::perms>(::archive_entry_perm(entry_));
        }
        Error setInformationFromFile(std::filesystem::path const& path)
        {
            if (!std::filesystem::exists(path))
                return Error{ARCHIVE_FAILED, "File does not exist"};

            setPathname(path.filename());
            setSize(std::filesystem::file_size(path));
            auto status = std::filesystem::status(path);
            Type type;
            switch (status.type())
            {
                case (std::filesystem::file_type::regular):
                {
                    type = Type::RegularFile;
                    break;
                }
                case (std::filesystem::file_type::directory):
                {
                    type = Type::Directory;
                    break;
                }
                case (std::filesystem::file_type::symlink):
                {
                    type = Type::SymbolicLink;
                    break;
                }
                case (std::filesystem::file_type::fifo):
                {
                    type = Type::Pipe;
                    break;
                }
                default:
                    return Error{ARCHIVE_FAILED, "File type not supported"};
            }
            setType(type);
            setPermissions(status.permissions());
            return Error{ARCHIVE_OK};
        }

      private:
        ::archive_entry* entry_;
    };
}
