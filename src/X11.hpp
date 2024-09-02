#pragma once

#include "application.hpp"
#include "src/window.hpp"
#include <X11/Xlib.h>
#include <unordered_map>

typedef Window X_Window;

namespace sg {

class X11Window : public Window {
public:
  X11Window(Display *dpy, X_Window win);
  ~X11Window() override;

  void drawRectangle(int x, int y, unsigned int width, unsigned int height,
                     const Color &color, unsigned int lineWidth) override;
  void fillRectangle(int x, int y, unsigned int width, unsigned int height,
                     const Color &color) override;

  void notifyDestroyed();

private:
  Display *dpy;
  X_Window win;
  GC gc;
};

class X11Application : public Application {
public:
  X11Application();
  X11Application(Display *dpy, X_Window rootWindow);
  void run() override;
  std::shared_ptr<Window> createWindow(const WindowInit &init) override;
  std::shared_ptr<Window> createCenteredWindow(const WindowInit &init) override;
  WindowDims getRootDimensions() override;
  ~X11Application() override;

private:
  void renderWindowIfExists(X_Window xwin);
  Display *dpy;
  X_Window root;
  bool ownedDpy = false;
  std::unordered_map<X_Window, std::shared_ptr<X11Window>> windows;
};

} // namespace sg
