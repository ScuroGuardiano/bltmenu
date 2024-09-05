#include "src/X11.hpp"
#include "src/color.hpp"
#include "src/window.hpp"

int main() {
  sg::X11Application app;
  
  sg::WindowInit init;
  init.width = 200;
  init.height = 200;
  init.borderColor = sg::rgb(0xff, 0x0, 0x0);
  init.backgroundColor = sg::rgb(0xaa, 0xaa, 0xaa);
  init.borderWidth = 4;
  init.klass = "BloatedMenu";

  auto win = app.createCenteredWindow(init);
  std::vector<sg::Font> fonts {
    sg::Font { "monospace", 14 }
  };
  float lineHeight = 1.5;

  win->setFonts(fonts);

  win->render = [&](sg::Window* win) {
    win->drawRectangle(50, 50, 100, 100, sg::rgb(0, 0xff, 0), 4);
    win->fillRectangle(75, 75, 50, 50, sg::rgb(0, 0, 0xff));
    win->drawText(0, 0, 200, 14, "Not fitting text with ellipsis", sg::rgb(0x0, 0x0, 0x0), true);
    win->drawText(0, (int)(lineHeight * fonts[0].size), 200, 14, "Not fitting text without ellipsis", sg::rgb(0x0, 0x0, 0x0), false);
  };

  app.run();
}
