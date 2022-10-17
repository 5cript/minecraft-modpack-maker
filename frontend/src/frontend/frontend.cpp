#include <frontend/api/minecraft.hpp>
#include <frontend/api/modrinth.hpp>
#include <frontend/config.hpp>
#include <frontend/modpack.hpp>
#include <frontend/page_model.hpp>

// Other Nui
#include <nui/core.hpp>
#include <nui/frontend/dom/dom.hpp>
#include <nui/frontend/event_system/event_context.hpp>
#include <nui/frontend/filesystem/file_dialog.hpp>
#include <nui/window.hpp>

// Elements
#include <nui/frontend/elements/body.hpp>
#include <nui/frontend/elements/button.hpp>
#include <nui/frontend/elements/div.hpp>
#include <nui/frontend/elements/img.hpp>
#include <nui/frontend/elements/label.hpp>
#include <nui/frontend/elements/span.hpp>

// Attributes
#include <nui/frontend/attributes/class.hpp>
#include <nui/frontend/attributes/for.hpp>
#include <nui/frontend/attributes/id.hpp>
#include <nui/frontend/attributes/on_click.hpp>
#include <nui/frontend/attributes/on_input.hpp>
#include <nui/frontend/attributes/place_holder.hpp>
#include <nui/frontend/attributes/read_only.hpp>
#include <nui/frontend/attributes/src.hpp>
#include <nui/frontend/attributes/style.hpp>
#include <nui/frontend/attributes/type.hpp>

// Components
#include <frontend/components/labeled_select.hpp>
#include <frontend/components/labeled_text_input.hpp>
#include <nui/frontend/components/select.hpp>

#include <iostream>

using namespace Nui;
using namespace Nui::Elements;
using namespace Nui::Attributes;
using namespace Nui::Attributes::Literals;

class MainPage
{
  public:
    MainPage()
    {
        loadConfig();
        loadMinecraftVersions(false /*TODO: add option somewhere*/);
        loadModLoaders();
        openModPack();
    }

  private:
    void loadMinecraftVersions(bool includeSnapshots)
    {
        auto response = Minecraft::getAllVersions();
        if (response.code != 200 || !response.body)
        {
            Console::error("Failed to load Minecraft versions: ", response.code);
            return;
        }
        decltype(response.body->versions) filteredVersions;
        if (includeSnapshots)
            filteredVersions = std::move(response.body->versions);
        else
        {
            std::copy_if(
                std::begin(response.body->versions),
                std::end(response.body->versions),
                std::back_inserter(filteredVersions),
                [](auto const& versionStruct) {
                    return versionStruct.type == "release";
                });
        }
        std::transform(
            std::begin(filteredVersions),
            std::end(filteredVersions),
            std::back_inserter(model_.availableMinecraftVersions.value()),
            [](auto const& versionStruct) {
                return Components::SelectOptions<std::string>{versionStruct.id, versionStruct.id};
            });
    }

    void loadModLoaders()
    {
        auto pushAvailableModLoader = [this](std::string const& ver) {
            model_.availableModLoaders.value().push_back({ver, ver});
        };
        pushAvailableModLoader("Forge");
        pushAvailableModLoader("Fabric");
    }

    void saveConfig()
    {
        ::saveConfig(config_, [](bool success) {
            Console::log("Config was saved.");
        });
    }
    void loadConfig()
    {
        ::loadConfig([this](Config&& cfg) {
            config_ = std::move(cfg);
        });
    }
    void openModPack()
    {
        modPack_.open({config_.openPack.value()}, [this]() {
            if (!modPack_.minecraftVersion().empty())
            {
                if (auto select = minecraftVersionSelect_.lock(); select)
                    select->val().set("value", modPack_.minecraftVersion());
            }
            if (!modPack_.modLoader().empty())
            {
                if (auto select = modLoaderSelect_.lock(); select)
                    select->val().set("value", modPack_.modLoader());
            }
        });
    }

    void searchMatchingMods()
    {
        if (debouncedSearch_.typeOf().as<std::string>() == "undefined")
        {
            debouncedSearch_ = emscripten::val::global("_").call<emscripten::val>(
                "debounce",
                Nui::bind([this, &value = searchFieldValue_.value()]() {
                    const auto results = Modrinth::Projects::search(Modrinth::Projects::SearchOptions{
                        .query = value,
                        .facets =
                            {
                                .versions = std::vector<std::string>{modPack_.minecraftVersion()},
                                .categories = std::vector<std::string>{modPack_.modLoader()},
                                .project_type = std::vector<std::string>{"mod"},
                            },
                    });
                    if (results.code == 200)
                    {
                        {
                            auto proxy = model_.searchHits.modify();
                            proxy.value() = results.body->hits;
                        }
                        globalEventContext.executeActiveEventsImmediately();
                    }
                }),
                500);
        }
        debouncedSearch_();
    }

    void onSelectSearchedMod(auto const& mod)
    {
        {
            auto proxy = searchFieldValue_.modify();
            proxy.value().clear();
        }
        globalEventContext.executeActiveEventsImmediately();
    }

  public:
    auto render()
    {
        using Nui::Elements::div;
        using namespace Nui::Components;

        int preselectModLoader = 0;
        int preselectMinecraftVersion = 0;
        if (!modPack_.minecraftVersion().empty())
        {
            auto it = std::find_if(
                std::begin(model_.availableMinecraftVersions.value()),
                std::end(model_.availableMinecraftVersions.value()),
                [this](auto const& opt) {
                    return opt.value == modPack_.minecraftVersion();
                });
            if (it != std::end(model_.availableMinecraftVersions.value()))
                preselectMinecraftVersion = std::distance(std::begin(model_.availableMinecraftVersions.value()), it);
        }
        if (!modPack_.modLoader().empty())
        {
            auto it = std::find_if(
                std::begin(model_.availableModLoaders.value()),
                std::end(model_.availableModLoaders.value()),
                [this](auto const& opt) {
                    return opt.value == modPack_.modLoader();
                });
            if (it != std::end(model_.availableModLoaders.value()))
                preselectModLoader = std::distance(std::begin(model_.availableModLoaders.value()), it);
        }

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
                    value = config_.openPack
                ),
                button{
                    type = "button",
                    class_ = "btn btn-primary",
                    id = "packOpenButton",
                    onClick = [this](emscripten::val event) {
                        FileDialog::showDirectoryDialog({}, [this](std::optional<std::vector<std::filesystem::path>> result) {
                            if (result)
                            {
                                config_.openPack = result.value()[0].string();
                                saveConfig();
                                openModPack();
                            }
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
                    selectModel = model_.availableMinecraftVersions,
                    preSelect = preselectMinecraftVersion,
                    onSelect = [this](int index) {
                        modPack_.minecraftVersion(model_.availableMinecraftVersions.value()[index].value);
                    },
                    selectReference = [this](std::weak_ptr<Dom::BasicElement>&& weak){
                        minecraftVersionSelect_ = std::move(weak);
                    }
                ),
                
                LabeledSelect(
                    {
                        .boxId = "modLoaderSelect",
                        .selectId = "modLoader",
                        .labelId = "modLoaderLabel",
                        .label = "Mod Loader"
                    },
                    selectModel = model_.availableModLoaders,
                    preSelect = preselectModLoader,
                    onSelect = [this](int index) {
                        modPack_.modLoader(model_.availableModLoaders.value()[index].value);
                    },
                    selectReference = [this](auto&& weak){
                        modLoaderSelect_ = std::move(weak);
                    }
                )
            ),
            // Mod Table Area
            div{
                id = "modTableArea"
            }(
                div{
                    id = "modAddBox"
                }(
                    label{}("Search for mod to add:"),
                    div{
                        id = "modInputBox"
                    }(
                        input{
                            type = "text",
                            id = "modSearchInput",
                            placeHolder = "Enter a mod name",
                            value = searchFieldValue_,
                            onInput = [this](emscripten::val event) {
                                searchFieldValue_ = event["target"]["value"].as<std::string>();
                                searchMatchingMods();
                            }
                        }(),
                        button{
                            type = "button",
                            class_ = "btn btn-primary",
                            id = "modSearchButton",
                            onClick = [](emscripten::val event) {
                                std::cout << "on Mod Submit\n";
                            }
                        }("Add")
                    ),
                    div{
                        id = "modSearchOverlay",
                        style = Style{
                            "visibility"_style = observe(searchFieldValue_).generate([this](){
                                if (searchFieldValue_->empty())
                                    return "hidden";
                                else
                                    return "visible";
                            }),
                        }
                    }(
                        range(model_.searchHits),
                        [this](auto i, auto const& hit) {
                            return div{
                                class_ = "mod-search-result",
                                onClick = [this, hit](emscripten::val) {
                                    onSelectSearchedMod(hit);
                                }
                            }(
                                img {
                                    src = hit.icon_url,
                                    onClick = [this, hit](emscripten::val) {
                                        onSelectSearchedMod(hit);
                                    }
                                }(),
                                span{
                                    onClick = [this, hit](emscripten::val) {
                                        onSelectSearchedMod(hit);
                                    }
                                }(
                                    hit.title
                                )
                            );
                        }
                    )
                )
            )
        );
        // clang-format on
    }

  private:
    MainPageModel model_;
    Config config_;
    ModPackManager modPack_;
    std::weak_ptr<Dom::BasicElement> minecraftVersionSelect_;
    std::weak_ptr<Dom::BasicElement> modLoaderSelect_;
    emscripten::val debouncedSearch_;
    Observed<std::string> searchFieldValue_;
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