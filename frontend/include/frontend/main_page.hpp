#pragma once

#include <frontend/components/mod_picker.hpp>
#include <frontend/config.hpp>
#include <frontend/modpack.hpp>
#include <frontend/page_model.hpp>

// Other Nui
#include <nui/core.hpp>
#include <nui/frontend/api/throttle.hpp>
#include <nui/frontend/api/timer.hpp>
#include <nui/frontend/dom/element.hpp>
#include <nui/frontend/event_system/event_context.hpp>
#include <nui/frontend/filesystem/file_dialog.hpp>
#include <nui/frontend/generator_typedefs.hpp>
#include <nui/window.hpp>

// Components
#include <nui/frontend/components/dialog.hpp>

class MainPage
{
  public:
    MainPage();

    Nui::ElementRenderer render();

  private:
    template <typename T, typename U>
    void showYesNoDialog(std::string const& body, T&& onYesAction, U&& onNoAction)
    {
        dialog_.setOnButtonClicked(
            [onYesAction = std::forward<T>(onYesAction), onNoAction = std::forward<U>(onNoAction)](auto button) {
                switch (button)
                {
                    case (Nui::Components::DialogController::Button::Yes):
                        onYesAction();
                        break;
                    case (Nui::Components::DialogController::Button::No):
                        onNoAction();
                        break;
                    default:
                        break;
                }
            });
        dialog_.setBody(body);
        dialog_.showModal();
    }
    template <typename T>
    void showYesNoDialog(std::string const& body, T&& onYesAction)
    {
        showYesNoDialog(body, std::forward<T>(onYesAction), []() {});
    }

    void loadMinecraftVersions(bool includeSnapshots);
    void loadModLoaders();
    void saveConfig();
    void loadConfig(std::function<void()> onAfterLoad);
    void openModPack();
    void searchMatchingModsImpl();
    void searchMatchingMods();
    void onSelectSearchedMod(auto const& searchHit);
    bool compareDates(std::string const& leftStamp, std::string const& rightStamp) const;

  private:
    Nui::ElementRenderer modTableArea();
    Nui::ElementRenderer modTable();
    Nui::ElementRenderer packControls();
    void updateLatestVersions();

  private:
    Nui::Window window_;
    MainPageModel model_;
    Config config_;
    ModPackManager modPack_;
    std::weak_ptr<Nui::Dom::BasicElement> minecraftVersionSelect_;
    std::weak_ptr<Nui::Dom::BasicElement> modLoaderSelect_;
    Nui::ThrottledFunction debouncedSearch_;
    Nui::Observed<std::string> searchFieldValue_;
    Nui::Components::DialogController dialog_;
    std::vector<std::string> minecraftVersions_;
    ModPickerController modPicker_;
    Nui::Observed<std::vector<Modrinth::Projects::Version>> lastModVersions_;
    Nui::ThrottledFunction modUpdateThrottle_;

    // mod version update
    Nui::Observed<bool> updateControlLock_{false};
    Nui::Observed<int> versionsToFetch_{0};
    Nui::Observed<int> versionsFetched_{0};

    // switches
    Nui::Observed<bool> featuredVersionsOnly_{false};
    Nui::Observed<bool> fuzzyMinecraftVersion_{false};
};