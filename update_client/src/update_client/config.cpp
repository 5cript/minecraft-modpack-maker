#include "config.hpp"
#include "base_path.hpp"

#include <fstream>
#include <iostream>

using json = nlohmann::json;

Config loadConfig(std::filesystem::path const& selfDirectory)
{
    std::ifstream reader{getBasePath(selfDirectory) / "updater.json", std::ios_base::binary};
    if (!reader.good())
        return {};
    try 
    {
        json j;
        reader >> j;
        Config conf;
        j.get_to(conf);
        return conf;
    }   
    catch(std::exception const& exc)
    {
        std::cout << "Could not parse config: " << exc.what() << "\n";
        std::rethrow_exception(std::current_exception());
    }
}

void saveConfig(std::filesystem::path const& selfDirectory, Config const& config)
{
    std::ofstream writer{getBasePath(selfDirectory) / "updater.json", std::ios_base::binary};
    if (!writer.good())
        return;
    json j = config;
    writer << std::setw(4) << j << std::endl;
}