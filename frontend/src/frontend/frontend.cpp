#include <frontend/api/minecraft.hpp>
#include <frontend/api/modrinth.hpp>
#include <frontend/config.hpp>
#include <frontend/modpack.hpp>
#include <frontend/page_model.hpp>

// Other Nui
#include <nui/core.hpp>
#include <nui/frontend/api/throttle.hpp>
#include <nui/frontend/dom/dom.hpp>
#include <nui/frontend/event_system/event_context.hpp>
#include <nui/frontend/filesystem/file.hpp>
#include <nui/frontend/filesystem/file_dialog.hpp>
#include <nui/window.hpp>

// Elements
#include <nui/frontend/elements/body.hpp>
#include <nui/frontend/elements/button.hpp>
#include <nui/frontend/elements/div.hpp>
#include <nui/frontend/elements/form.hpp>
#include <nui/frontend/elements/img.hpp>
#include <nui/frontend/elements/label.hpp>
#include <nui/frontend/elements/span.hpp>
#include <nui/frontend/elements/td.hpp>
#include <nui/frontend/elements/th.hpp>
#include <nui/frontend/elements/tr.hpp>

// Attributes
#include <nui/frontend/attributes/class.hpp>
#include <nui/frontend/attributes/col_span.hpp>
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
#include <nui/frontend/components/dialog.hpp>
#include <nui/frontend/components/select.hpp>
#include <nui/frontend/components/table.hpp>

#include <iostream>
#include <tuple>

using namespace Nui;
using namespace Nui::Elements;
using namespace Nui::Attributes;
using namespace Nui::Components;
using namespace Nui::Attributes::Literals;

class MainPage
{
  public:
    MainPage()
        : window_{}
        , model_{}
        , config_{}
        , modPack_{}
        , minecraftVersionSelect_{}
        , modLoaderSelect_{}
        , debouncedSearch_{}
        , searchFieldValue_{}
        , dialog_{DialogController::ConstructionArgs{
              .className = "page-dialog",
              .title = "Please Confirm",
              .body = "Are you sure you want to do this?",
              .buttonClassName = "btn btn-primary",
              .buttonConfiguration = DialogController::ButtonConfiguration::YesNo,
              .onButtonClicked =
                  [](DialogController::Button button) {
                      std::cout << "Dialog opened without attached action!";
                  },
          }}
    {
        loadModLoaders();
        loadConfig([this]() {
            loadMinecraftVersions(false /*TODO: add option somewhere*/);
        });

        Nui::throttle(
            500,
            [this]() {
                searchMatchingModsImpl();
            },
            [this](ThrottledFunction&& func) {
                debouncedSearch_ = std::move(func);
            },
            true);
    }

  private:
    template <typename T, typename U>
    void showYesNoDialog(std::string const& body, T&& onYesAction, U&& onNoAction)
    {
        dialog_.setOnButtonClicked(
            [onYesAction = std::forward<T>(onYesAction), onNoAction = std::forward<U>(onNoAction)](auto button) {
                switch (button)
                {
                    case (DialogController::Button::Yes):
                        onYesAction();
                        break;
                    case (DialogController::Button::No):
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

    void loadMinecraftVersions(bool includeSnapshots)
    {
        Minecraft::getAllVersions([this, includeSnapshots](auto const& response) {
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
            {
                auto proxy = model_.availableMinecraftVersions.modify();
                std::transform(
                    std::begin(filteredVersions),
                    std::end(filteredVersions),
                    std::back_inserter(proxy.value()),
                    [](auto const& versionStruct) {
                        return Components::SelectOptions<std::string>{versionStruct.id, versionStruct.id};
                    });
            }
            globalEventContext.executeActiveEventsImmediately();
            openModPack();
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
    void loadConfig(std::function<void()> onAfterLoad)
    {
        ::loadConfig([this, onAfterLoad = std::move(onAfterLoad)](Config&& cfg) {
            {
                config_ = std::move(cfg);
            }
            globalEventContext.executeActiveEventsImmediately();
            onAfterLoad();
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
            globalEventContext.executeActiveEventsImmediately();
        });
    }

    void searchMatchingModsImpl()
    {
        Modrinth::Projects::search(
            Modrinth::Projects::SearchOptions{
                .query = searchFieldValue_.value(),
                .facets =
                    {
                        .versions = std::vector<std::string>{modPack_.minecraftVersion()},
                        .categories = std::vector<std::string>{modPack_.modLoader()},
                        .project_type = std::vector<std::string>{"mod"},
                    },
            },
            [this](Http::Response<Modrinth::Projects::SearchResult> const& results) {
                if (results.code == 200)
                {
                    {
                        auto proxy = model_.searchHits.modify();
                        proxy.value() = results.body->hits;
                    }
                    globalEventContext.executeActiveEventsImmediately();
                }
            });
    }

    void searchMatchingMods()
    {
        debouncedSearch_();
    }

    void onSelectSearchedMod(auto const& searchHit)
    {
        using namespace std::string_literals;

        auto options = Modrinth::prepareOptions();
        options.verbose = true;
        options.followRedirects = true;
        options.autoReferer = true;
        options.dontDecodeBody = true;
        Http::get<std::string>(searchHit.icon_url, options, [this, searchHit](auto const& response) {
            std::cout << searchHit.icon_url << "\n";
            modPack_.addMod({
                .name = searchHit.title,
                .id = searchHit.project_id,
                .slug = searchHit.slug,
                .installedName = "",
                .installedTimestamp = "",
                .logoPng64 = "data:image/png;base64,"s + *response.body,
                .newestTimestamp = searchHit.date_modified // FIXME: this is probably not the correct date.
            });
            {
                auto proxy = searchFieldValue_.modify();
                proxy.value().clear();
            }
            globalEventContext.executeActiveEventsImmediately();
        });
    }

    bool compareDates(std::string const& leftStamp, std::string const& rightStamp) const
    {
        return emscripten::val::global("compareDates")(emscripten::val{leftStamp}, emscripten::val{rightStamp})
            .as<bool>();
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
            // Dialogs
            Dialog(dialog_),

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
                    form{
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
                        }()
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
                                onClick = [this, hit](emscripten::val event) {
                                    event.call<void>("stopPropagation");
                                    onSelectSearchedMod(hit);
                                }
                            }(
                                img {
                                    src = hit.icon_url,
                                    onClick = [this, hit](emscripten::val event) {
                                        event.call<void>("stopPropagation");
                                        onSelectSearchedMod(hit);
                                    }
                                }(),
                                span{
                                    onClick = [this, hit](emscripten::val event) {
                                        event.call<void>("stopPropagation");
                                        onSelectSearchedMod(hit);
                                    }
                                }(
                                    hit.title
                                )
                            );
                        }
                    )
                ),
                Table{
                    tableModel = modPack_.mods(),
                    headerRenderer = [](){
                        return tr{}(
                            th{}("Controls"),
                            th{}("Name"),
                            th{}("Installed Date")
                        );
                    },
                    rowRenderer = [this](auto i, auto const& mod) -> Nui::ElementRenderer{
                        const bool isOutdated = compareDates(mod.installedTimestamp, mod.newestTimestamp);

                        auto cellClass = [&mod, isOutdated](){
                            if (mod.installedTimestamp.empty())
                                return "table-cell not-installed-cell";
                            if (isOutdated)
                                return "table-cell outdated-cell";
                            return "table-cell installed-cell";                            
                        };

                        return fragment(
                            td{
                                class_ = cellClass()
                            }(
                                div{
                                    class_ = "remove-button",
                                    onClick = [this, id = mod.id](){
                                        showYesNoDialog("Are you sure you want to remove this mod?", [this, id](){
                                            modPack_.removeMod(id);
                                        });
                                    }
                                }(),
                                div{
                                    class_ = "install-button",
                                    onClick = [this, id = mod.id, isOutdated](){
                                        if (isOutdated) {
                                            showYesNoDialog("Update the mod? This could cause issues.", [this, id](){
                                                modPack_.installMod(id);
                                            });
                                        }
                                        else
                                            modPack_.installMod(id);
                                    }
                                }
                            ),
                            td{
                                class_ = cellClass()
                            }(                                
                                img{
                                    src = mod.logoPng64,
                                    class_ = "table-mod-icon"
                                }(),
                                span{}(mod.name)
                            ),
                            td{
                                class_ = cellClass()
                            }(
                                [&mod]() -> std::string {
                                    if (mod.installedTimestamp.empty())
                                        return "Not Installed";
                                    return mod.installedTimestamp;
                                }
                            )
                        );
                    },
                    footerRenderer = []() -> Nui::ElementRenderer{
                        return tr{}(
                            td{
                                colSpan = 4,
                                class_ = "footer-cell"
                            }()
                        );
                    }
                }(
                    tableAttributes = std::make_tuple(
                        id = "modTable"
                    )
                )
            )
        );
        // clang-format on
    }

  private:
    Window window_;
    MainPageModel model_;
    Config config_;
    ModPackManager modPack_;
    std::weak_ptr<Dom::BasicElement> minecraftVersionSelect_;
    std::weak_ptr<Dom::BasicElement> modLoaderSelect_;
    ThrottledFunction debouncedSearch_;
    Observed<std::string> searchFieldValue_;
    Nui::Components::DialogController dialog_;
};

extern "C" {
    void EMSCRIPTEN_KEEPALIVE frontendMain()
    {
        thread_local MainPage page;
        thread_local Dom::Dom dom;
        dom.setBody(page.render());
    }
    int EMSCRIPTEN_KEEPALIVE main(int argc, char* argv[])
    {
        frontendMain();
        return 0;
    }
}

EMSCRIPTEN_BINDINGS(nui_example_frontend)
{
    emscripten::function("main", &frontendMain);
}
#include <nui/frontend/bindings.hpp>