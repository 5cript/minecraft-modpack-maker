#include <frontend/main_page.hpp>

#include <nui/frontend/dom/dom.hpp>

extern "C" {
    void EMSCRIPTEN_KEEPALIVE frontendMain()
    {
        thread_local MainPage page;
        thread_local Nui::Dom::Dom dom;
        dom.setBody(page.render());
    }
}

EMSCRIPTEN_BINDINGS(nui_example_frontend)
{
    emscripten::function("main", &frontendMain);
}
#include <nui/frontend/bindings.hpp>