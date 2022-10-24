#pragma once

#include <openssl/sha.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>

[[maybe_unused]] static std::string sha256FromFile(std::filesystem::path const& source)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    if (!SHA256_Init(&sha256))
        throw std::runtime_error("Could not initialize hash context.");

    std::ifstream reader{source, std::ios_base::binary};
    if (!reader.good())
        throw std::runtime_error("Could not open file to generate hash.");

    std::string buffer(4096, '\0');
    do
    {
        reader.read(buffer.data(), buffer.size());
        if (!SHA256_Update(&sha256, buffer.c_str(), reader.gcount()))
            throw std::runtime_error("Could not feed hash data");
    } while (static_cast<std::size_t>(reader.gcount()) == buffer.size());

    if (!SHA256_Final(hash, &sha256))
        throw std::runtime_error("Could not finalize hash");

    std::stringstream shastr;
    shastr << std::hex << std::setfill('0');
    for (const auto& byte : hash)
    {
        shastr << std::setw(2) << (int)byte;
    }
    return shastr.str();
}