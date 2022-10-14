#pragma once

#include <nui/frontend/components/model/select.hpp>
#include <nui/frontend/event_system/observed_value.hpp>

struct Modpack
{
    Nui::Observed<std::string> minecraftVersion;
};

struct MainPageModel
{
    Nui::Observed<std::string> packPath;
    Nui::Observed<std::vector<Nui::Components::SelectOptions<std::string>>> availableMinecraftVersions;
    Nui::Observed<std::vector<Nui::Components::SelectOptions<std::string>>> availableModLoaders;

    Modpack modpack;
};