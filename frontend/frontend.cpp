#include "page_model.hpp"

#include <nui/core.hpp>
#include <nui/window.hpp>

#include <nui/frontend/dom/dom.hpp>
#include <nui/frontend/filesystem/file_dialog.hpp>

// Elements
#include <nui/frontend/elements/body.hpp>
#include <nui/frontend/elements/button.hpp>
#include <nui/frontend/elements/div.hpp>
#include <nui/frontend/elements/label.hpp>

// Attributes
#include <nui/frontend/attributes/class.hpp>
#include <nui/frontend/attributes/for.hpp>
#include <nui/frontend/attributes/id.hpp>
#include <nui/frontend/attributes/on_click.hpp>
#include <nui/frontend/attributes/place_holder.hpp>
#include <nui/frontend/attributes/read_only.hpp>
#include <nui/frontend/attributes/type.hpp>

// Components
#include "components/labeled_select.hpp"
#include "components/labeled_text_input.hpp"
#include <nui/frontend/components/select.hpp>

#include <iostream>

using namespace Nui;
using namespace Nui::Elements;
using namespace Nui::Attributes;

class MainPage
{
  public:
    MainPage()
    {
        auto pushAvailableMcVersion = [this](std::string const& ver) {
            model.availableMinecraftVersions.value().push_back({ver, ver});
        };

        auto pushAvailableModLoader = [this](std::string const& ver) {
            model.availableModLoaders.value().push_back({ver, ver});
        };

        // TODO: load from API.
        pushAvailableMcVersion("1.18.1");
        pushAvailableMcVersion("1.18.0");
        pushAvailableMcVersion("1.17.1");
        pushAvailableMcVersion("1.16.5");
        pushAvailableMcVersion("1.16.4");

        pushAvailableModLoader("Forge");
        pushAvailableModLoader("Fabric");
    }

    auto render()
    {
        using Nui::Elements::div;
        using namespace Nui::Components;

        // clang-format off
        return body{}(
            // Opened Pack
            div{
                id = "openedPackBox"
            }(
                LabeledTextInput(
                    LabeledTextInputArgs{
                        .boxId = "openPackInput",
                        .inputId = "packPath",
                        .labelId = "openedPackLabel",
                        .label = "Opened Pack"
                    },
                    value = model.packPath,
                    readOnly = true
                ),
                button{
                    type = "button",
                    class_ = "btn btn-primary",
                    id = "packOpenButton",
                    onClick = [](emscripten::val event) {
                        std::cout << "onClick\n";
                        FileDialog::showDirectoryDialog({}, [](std::optional<std::vector<std::filesystem::path>> result) {
                            if (result)
                                std::cout << "pack opened: " << result->front().string() << std::endl;
                            else
                                std::cout << "pack open canceled" << std::endl;
                        });
                    }
                }("Open")
            ),

            // Pack Options
            div{
                id = "packOptions"
            }(
                LabeledSelect(
                    {
                        .boxId = "minecraftVersionSelect",
                        .selectId = "minecraftVersion",
                        .labelId = "minecraftVersionLabel",
                        .label = "Minecraft Version"
                    },
                    selectModel = model.availableMinecraftVersions,
                    preSelect = 0,
                    onSelect = [this](int index) {
                        std::cout << "Selected Minecraft Version " << model.availableMinecraftVersions.value()[index].value << "\n";
                    }
                ),
                
                // FIXME: reusing the range is a problem because the range context is per obs value and therefore a rendering will not happen. must fix.
                LabeledSelect(
                    {
                        .boxId = "modLoaderSelect",
                        .selectId = "modLoader",
                        .labelId = "modLoaderLabel",
                        .label = "Mod Loader"
                    },
                    selectModel = model.availableModLoaders,
                    preSelect = 0,
                    onSelect = [this](int index) {
                        std::cout << "Selected Mod Loader " << model.availableModLoaders.value()[index].value << "\n";
                    }
                )
            )
        );
        // clang-format on
    }

  private:
    MainPageModel model;
};

void frontendMain()
{
    thread_local MainPage page;

    thread_local Dom::Dom dom;
    dom.setBody(page.render());
}

EMSCRIPTEN_BINDINGS(nui_example_frontend)
{
    emscripten::function("main", &frontendMain);
}
#include <nui/frontend/bindings.hpp>