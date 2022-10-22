#pragma once

#include <archive.h>

#include <exception>
#include <string>

namespace Archive
{
    class Error : public std::exception
    {
      public:
        explicit Error(::archive* archive, int libarchiveCode);
        explicit Error(::archive* archive, int libarchiveCode, std::string const& extraMessage);
        explicit Error(int libarchiveCode);
        explicit Error(int libarchiveCode, std::string const& extraMessage);
        int code() const noexcept;
        explicit operator bool() const noexcept;
        char const* what() const noexcept override;
        std::string message() const;

      private:
        int code_;
        std::string message_;
    };
}
