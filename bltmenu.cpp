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

  win->render = [](sg::Window* win) {
    win->drawRectangle(50, 50, 100, 100, sg::rgb(0, 0xff, 0), 4);
    win->fillRectangle(75, 75, 50, 50, sg::rgb(0, 0, 0xff));
  };

  app.run();
}
