#include "X11.hpp"
#include "src/window.hpp"
#include <X11/X.h>
#include <X11/Xlib.h>
#include <cstdio>
#include <map>
#include <memory>

namespace sg {

static std::map<uint32_t, unsigned int> cachedColors;

unsigned int colorToPixel(Display *dpy, const Color &color) {
  uint32_t key = color.r + (color.g << 8) + (color.b << 16);
  if (cachedColors.count(key) == 1) {
    return cachedColors[key];
  }

  XColor sc, tc;
  // rgb:rr/bb/gg<NULL>
  char buf[4 + 3 + 3 + 2 + 1];
  std::snprintf(buf, sizeof(buf), "rgb:%02x/%02x/%02x", color.r, color.g,
                color.b);

  XAllocNamedColor(dpy, DefaultColormap(dpy, DefaultScreen(dpy)), buf, &sc,
                   &tc);
  cachedColors[key] = sc.pixel;

  return sc.pixel;
}

X11Window::X11Window(Display *dpy, X_Window win) {
  this->dpy = dpy;
  this->win = win;
  this->gc = XCreateGC(this->dpy, this->win, 0, nullptr);
}

X11Window::~X11Window() {
  XFreeGC(this->dpy, this->gc);

  if (this->win) {
    XDestroyWindow(this->dpy, this->win);
  }
}

void X11Window::drawRectangle(int x, int y, unsigned int width,
                              unsigned int height, const Color &color,
                              unsigned int lineWidth) {
  XSetForeground(this->dpy, this->gc, colorToPixel(this->dpy, color));
  XSetLineAttributes(this->dpy, this->gc, lineWidth, LineSolid, CapNotLast, JoinMiter);
  XDrawRectangle(this->dpy, this->win, this->gc, x, y, width, height);
}

void X11Window::fillRectangle(int x, int y, unsigned int width,
                              unsigned int height, const Color &color) {
  XSetForeground(this->dpy, this->gc, colorToPixel(this->dpy, color));
  XFillRectangle(this->dpy, this->win, this->gc, x, y, width, height);
}

void X11Window::notifyDestroyed() { this->win = 0; }

X11Application::X11Application() {
  this->dpy = XOpenDisplay(nullptr);
  this->root = DefaultRootWindow(this->dpy);
  this->ownedDpy = true;
}

X11Application::X11Application(Display *dpy, X_Window rootWindow) {
  this->dpy = dpy;
  this->root = rootWindow;

  // Yeah, I know it's alredy set to false.
  // I am paranoic ok? And I like that this constructor
  // explicitly says it doesn't own dpy.
  this->ownedDpy = false;
}

void X11Application::run() {
  XEvent ev;
  while (!XNextEvent(this->dpy, &ev) && this->windows.size() > 0) {
    switch (ev.type) {
    case MapNotify:
      this->renderWindowIfExists(ev.xmap.window);
      break;

    case Expose:
      this->renderWindowIfExists(ev.xexpose.window);
      break;

    case DestroyNotify:
      if (this->windows.count(ev.xdestroywindow.window)) {
        this->windows[ev.xdestroywindow.window]->notifyDestroyed();
        this->windows.erase(ev.xdestroywindow.window);
      }
    }
  }
}

std::shared_ptr<Window> X11Application::createWindow(const WindowInit &init) {
  XSetWindowAttributes swa;
  swa.override_redirect = True;
  swa.background_pixel = colorToPixel(this->dpy, init.backgroundColor);

  X_Window win = XCreateWindow(this->dpy, this->root, init.x, init.y,
                               init.width, init.height, init.borderWidth,
                               CopyFromParent, CopyFromParent, CopyFromParent,
                               CWOverrideRedirect | CWBackPixel, &swa);

  XSetWindowBorder(this->dpy, win, colorToPixel(this->dpy, init.borderColor));

  XSelectInput(this->dpy, win, StructureNotifyMask);
  XMapRaised(this->dpy, win);

  auto wrappedWindow = std::make_shared<X11Window>(this->dpy, win);
  this->windows[win] = wrappedWindow;
  return wrappedWindow;
}

std::shared_ptr<Window>
X11Application::createCenteredWindow(const WindowInit &init) {
  WindowInit copied = init;
  XWindowAttributes attrib;
  XGetWindowAttributes(this->dpy, this->root, &attrib);
  copied.x = (attrib.width - copied.width) / 2;
  copied.y = (attrib.height - copied.height) / 2;

  return this->createWindow(copied);
}

WindowDims X11Application::getRootDimensions() {
  XWindowAttributes attrib;
  XGetWindowAttributes(this->dpy, this->root, &attrib);
  WindowDims dims;
  dims.width = attrib.width;
  dims.height = attrib.height;

  return dims;
}

void X11Application::renderWindowIfExists(X_Window xwin) {
  if (this->windows.count(xwin)) {
    auto win = this->windows[xwin];
    if (win->render) {
      // Yeah, raw pointer here, because I don't need RC capabilities
      // in render function. It should be pure.
      win->render(win.get());
      XFlush(this->dpy);
    }
  }
}

X11Application::~X11Application() {
  if (ownedDpy) {
    XCloseDisplay(this->dpy);
  }
}

} // namespace sg
