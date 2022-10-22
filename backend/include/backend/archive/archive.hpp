#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#include <archive.h>
#pragma clang diagnostic pop

#include <type_traits>

namespace Archive
{
    class Archive
    {
      public:
        operator std::add_pointer_t<::archive>()
        {
            return m_archive;
        }
        operator std::add_pointer_t<::archive const>() const
        {
            return m_archive;
        }

        virtual ~Archive()
        {
            m_deleter(m_archive);
        }

      protected:
        Archive(::archive* arch, void (*deleter)(::archive*))
            : m_archive{arch}
            , m_deleter{deleter}
        {}

        ::archive* m_archive;
        void (*m_deleter)(::archive*);
    };

    class ArchiveWriter : public Archive
    {
      public:
        ArchiveWriter()
            : Archive{
                  ::archive_write_new(),
                  [](auto* arch) {
                      archive_write_close(arch);
                      archive_write_free(arch);
                  },
              }
        {}
    };

    class ArchiveReader : public Archive
    {
      public:
        ArchiveReader()
            : Archive{
                  ::archive_read_new(),
                  [](auto* arch) {
                      archive_read_close(arch);
                      archive_read_free(arch);
                  },
              }
        {}
    };
}
