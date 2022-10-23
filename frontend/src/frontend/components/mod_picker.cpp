#include <frontend/components/mod_picker.hpp>

#include <nui/frontend/attributes/class.hpp>
#include <nui/frontend/attributes/id.hpp>
#include <nui/frontend/attributes/method.hpp>
#include <nui/frontend/attributes/on_click.hpp>
#include <nui/frontend/components/table.hpp>
#include <nui/frontend/dom/reference.hpp>
#include <nui/frontend/elements/button.hpp>
#include <nui/frontend/elements/dialog.hpp>
#include <nui/frontend/elements/form.hpp>
#include <nui/frontend/elements/h1.hpp>
#include <nui/frontend/elements/p.hpp>

#include <iostream>

using namespace Nui;
using namespace Nui::Attributes;
using namespace Nui::Elements;
using namespace Nui::Components;

void ModPickerController::showModal()
{
    if (auto dialog = dialog_.lock())
        dialog->val().call<void>("showModal");
}
void ModPickerController::setDialog(std::weak_ptr<Nui::Dom::Element> dialog)
{
    dialog_ = std::move(dialog);
}
void ModPickerController::close()
{
    if (auto dialog = dialog_.lock())
        dialog->val().call<void>("close");
}

Nui::ElementRenderer modPicker(
    ModPickerController& controller,
    Observed<std::vector<Modrinth::Projects::Version>> const& modVersions,
    std::function<void(std::optional<Modrinth::Projects::Version> const&)> const& onPick)
{
    // clang-format off
    return dialog{
        id = "modPicker"
    }(
        Dom::reference([&controller](auto element){
            controller.setDialog(std::static_pointer_cast<Dom::Element>(element.lock()));
        }),
        form{
            method = "dialog"
        }(
            h1{}(
                "Pick mod to install."
            ),
            p{}(
                "Click on one of the table entries or click on cancel."
            ),
            Table{
                tableModel = modVersions,
                headerRenderer = [](){
                    return tr{}(
                        th{}("Name"),
                        th{}("Game Versions"),
                        th{}("Date Published"),
                        th{}("Downloads")
                    );
                },
                rowRenderer = [&controller, onPick](auto i, auto const& mod) -> Nui::ElementRenderer {
                    auto onCellClick = [mod, &controller, onPick](emscripten::val){
                        onPick(mod);
                        controller.close();
                    };

                    return tr{}(
                        td{
                            class_ = "picker-name-cell",
                            onClick = onCellClick
                        }(
                            mod.name
                        ),
                        td{
                            onClick = onCellClick
                        }(
                            [mod]() -> std::string{
                                std::string versions;
                                for (auto const& version : mod.game_versions)
                                    versions += version + ", ";
                                versions.pop_back();
                                versions.pop_back();
                                return versions;
                            }()                            
                        ),
                        td{
                            class_ = "date-published-cell",
                            onClick = onCellClick
                        }(
                            mod.date_published // TODO: prettier
                        ),
                        td{
                            onClick = onCellClick
                        }(
                            std::to_string(mod.downloads)
                        )
                    );
                }
            }(
                tableAttributes = std::make_tuple(
                    id = "pickerTable"
                )
            ),
            button{
                class_ = "btn btn-danger",
                onClick = [&controller, onPick](){
                    onPick(std::nullopt);
                    controller.close();
                }
            }(
                "Cancel"
            )
        )
    );
    // clang-format on
}