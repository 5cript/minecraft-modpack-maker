#pragma once

#include <nui/frontend/event_system/observed_value.hpp>
#include <nui/frontend/generator_typedefs.hpp>

#include <frontend/api/modrinth.hpp>

#include <functional>

class ModPickerController
{
  public:
    void showModal(std::string const& modName);
    void setDialog(std::weak_ptr<Nui::Dom::Element> dialog);
    void close();
    Nui::Observed<std::string>& modName();

  private:
    std::weak_ptr<Nui::Dom::Element> dialog_;
    Nui::Observed<std::string> modName_;
};

Nui::ElementRenderer modPicker(
    ModPickerController& controller,
    Nui::Observed<std::vector<Modrinth::Projects::Version>> const& modVersions,
    std::function<void(std::optional<Modrinth::Projects::Version> const&)> const& onPick);