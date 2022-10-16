#pragma once

#include <nui/frontend/event_system/observed_value.hpp>
#include <nui/frontend/utility/val_conversion.hpp>

#include <functional>

struct Config
{
    Nui::Observed<std::string> openPack;
};
BOOST_DESCRIBE_STRUCT(Config, (), (openPack));

void loadConfig(std::function<void(Config&&)> onLoad);
void saveConfig(Config const& config, std::function<void(bool)> onSaveComplete);