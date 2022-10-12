#include <nui/core.hpp>
#include <nui/window.hpp>

#include <nui/frontend/dom/dom.hpp>
#include <nui/frontend/elements.hpp>

void frontendMain() {
  using namespace Nui;
  using namespace Nui::Elements;

  const auto page = body{}("Hello World!");

  thread_local Dom::Dom dom;
  dom.setBody(page);
}

EMSCRIPTEN_BINDINGS(nui_example_frontend) {
  emscripten::function("main", &frontendMain);
}
#include <nui/frontend/bindings.hpp>