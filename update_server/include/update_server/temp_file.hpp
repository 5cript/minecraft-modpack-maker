#pragma once

#include <filesystem>
#include <fstream>

class TempFile
{
public:
    TempFile(std::filesystem::path const& filename)
        : path_{filename}
        , writer_{filename, std::ios_base::binary}
    {}

    void close()
    {
        writer_.close();
    }

    ~TempFile()
    {
        writer_.close();
        std::filesystem::remove(path_);
    }

    operator std::ofstream&()
    {
        return writer_;
    }

private:
    std::filesystem::path path_;
    std::ofstream writer_;
};