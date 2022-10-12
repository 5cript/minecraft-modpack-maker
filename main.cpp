#include <nui/core.hpp>
#include <nui/window.hpp>

// This file is generated by nui and the header name is the target name
#include <index.hpp>

int main() {
  using namespace Nui;

  Window window{"Window Title", true /* may open debug window */};
  window.setSize(480, 320, WebViewHint::WEBVIEW_HINT_NONE);
  window.setHtml(index());
  window.run();
}