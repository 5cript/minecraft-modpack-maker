#pragma once

// Elements
#include <nui/frontend/elements/form.hpp>
#include <nui/frontend/elements/label.hpp>

// Attributes
#include <nui/frontend/attributes/class.hpp>
#include <nui/frontend/attributes/for.hpp>
#include <nui/frontend/attributes/place_holder.hpp>
#include <nui/frontend/attributes/read_only.hpp>
#include <nui/frontend/attributes/type.hpp>

// Components
#include <nui/frontend/components/text_input.hpp>

struct LabeledTextInputArgs
{
    char const* boxId;
    char const* inputId;
    char const* labelId;
    char const* label;
};

constexpr auto LabeledTextInput(LabeledTextInputArgs&& options, auto&&... inputArgs)
{
    using namespace Nui::Elements;
    using namespace Nui::Attributes;
    using namespace Nui::Components;
    using Nui::Elements::div;

    // clang-format off
    return form{
        class_ = "form-floating boxed-input",
        id = options.boxId,
    }(
        TextInput(std::forward<std::decay_t<decltype(inputArgs)>>(inputArgs)..., id = options.inputId, class_ = "form-control input-sm")(),
        label{
            id = options.labelId,
            for_ = options.inputId,
        }(options.label)
    );
    // clang-format on
}