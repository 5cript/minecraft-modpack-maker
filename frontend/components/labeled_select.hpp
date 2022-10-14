#pragma once

#include <nui/frontend/components/model/select.hpp>
#include <nui/frontend/event_system/observed_value.hpp>

// Elements
#include <nui/frontend/elements/div.hpp>
#include <nui/frontend/elements/label.hpp>
#include <nui/frontend/elements/option.hpp>
#include <nui/frontend/elements/select.hpp>

// Attributes
#include <nui/frontend/attributes/class.hpp>
#include <nui/frontend/attributes/for.hpp>
#include <nui/frontend/attributes/place_holder.hpp>
#include <nui/frontend/attributes/read_only.hpp>
#include <nui/frontend/attributes/selected.hpp>
#include <nui/frontend/attributes/type.hpp>
#include <nui/frontend/attributes/value.hpp>

// Components
#include <nui/frontend/components/select.hpp>

struct LabeledSelectArgs
{
    char const* boxId;
    char const* selectId;
    char const* labelId;
    char const* label;
};

template <template <typename...> typename ContainerT, typename ValueType, typename PreSelect, typename OnSelect>
constexpr auto LabeledSelect(
    LabeledSelectArgs&& options,
    Nui::CustomAttribute<
        Nui::Observed<ContainerT<Nui::Components::SelectOptions<ValueType>>>&,
        Nui::Components::selectModelTag>&& selectModel,
    PreSelect preSelect,
    OnSelect onSelectFunc,
    auto&&... selectArgs)
{
    using namespace Nui::Elements;
    using namespace Nui::Attributes;
    using Nui::Elements::div;

    // clang-format off
    return div{
        class_ = "form-floating boxed-input",
        id = options.boxId,
    }(
        Select(
            std::move(selectModel),
            std::move(preSelect),
            std::move(onSelectFunc),
            id = options.selectId,
            class_ = "form-select",
            std::forward<std::decay_t<decltype(selectArgs)>>(selectArgs)...
        ),
        label{
            id = options.labelId,
            for_ = options.selectId,
        }(options.label)
    );
    // clang-format on
}