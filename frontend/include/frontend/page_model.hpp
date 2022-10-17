#pragma once

#include <frontend/api/modrinth.hpp>

#include <nui/frontend/components/model/select.hpp>
#include <nui/frontend/event_system/observed_value.hpp>

struct MainPageModel
{
    Nui::Observed<std::vector<Nui::Components::SelectOptions<std::string>>> availableMinecraftVersions;
    Nui::Observed<std::vector<Nui::Components::SelectOptions<std::string>>> availableModLoaders;

    Nui::Observed<std::vector<Modrinth::Projects::SearchHit>> searchHits;
};