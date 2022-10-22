#include <backend/archive/error.hpp>

namespace Archive
{
    Error::Error(::archive* archive, int libarchiveCode)
        : code_{libarchiveCode}
        , message_{std::string{::archive_error_string(archive)}}
    {
        ::archive_clear_error(archive);
    }
    Error::Error(::archive* archive, int libarchiveCode, std::string const& extraMessage)
        : code_{libarchiveCode}
        , message_{std::string{::archive_error_string(archive)} + " (" + extraMessage + ')'}
    {
        ::archive_clear_error(archive);
    }
    Error::Error(int libarchiveCode)
        : code_{libarchiveCode}
        , message_{"Error code " + std::to_string(libarchiveCode)}
    {}
    Error::Error(int libarchiveCode, std::string const& extraMessage)
        : code_{libarchiveCode}
        , message_{extraMessage}
    {}
    int Error::code() const noexcept
    {
        return code_;
    }
    Error::operator bool() const noexcept
    {
        return code_ != 0;
    }
    char const* Error::what() const noexcept
    {
        return message_.c_str();
    }
    std::string Error::message() const
    {
        return message_;
    }
}
