#pragma once

#include "color.hpp"
#include <functional>
#include <string>
namespace sg {

struct ScreenDims {
  int x;
  int y;
  unsigned int width;
  unsigned int height;
};

struct WindowInit {
  int x;
  int y;
  unsigned int width;
  unsigned int height;
  unsigned int borderWidth;
  Color backgroundColor;
  Color borderColor;
  std::string klass;
};

class Window {
public:
  /// Render function will be called everytime when Window will be able to
  /// or will need to render window contents. Render funtion should be pure.
  /// Pointer to window has lifetime of render function.
  /// Do not store it anywhere outside render function, you will die!
  std::function<void(Window *)> render = nullptr;

  virtual ~Window(){};
  virtual void drawRectangle(int x, int y, unsigned int width,
                             unsigned int height, const Color &color,
                             unsigned int lineWidth) = 0;
  virtual void fillRectangle(int x, int y, unsigned int width,
                             unsigned int height, const Color &color) = 0;
};

} // namespace sg
