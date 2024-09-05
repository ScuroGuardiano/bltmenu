#include "X11.hpp"
#include "src/window.hpp"
#include <X11/X.h>
#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>
#include <cstdio>
#include <map>
#include <memory>
#include <sstream>

// I took this code straight from dmenu
// Idk how it exactly works but it works
#ifdef XINERAMA

#include <X11/extensions/Xinerama.h>

#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define INTERSECT(x, y, w, h, r)                                               \
  (MAX(0, MIN((x) + (w), (r).x_org + (r).width) - MAX((x), (r).x_org)) *       \
   MAX(0, MIN((y) + (h), (r).y_org + (r).height) - MAX((y), (r).y_org)))

#endif

bool getXineramaScreenDims(Display *dpy, X_Window root,
                           sg::DimsAndPos *sd_out) {
#ifdef XINERAMA
  XineramaScreenInfo *info;
  Window pw, w, dw, *dws;
  XWindowAttributes wa;
  int x, y, a, di, i = 0, n, area = 0;
  unsigned int du;

  if ((info = XineramaQueryScreens(dpy, &n))) {
    XGetInputFocus(dpy, &w, &di);
    if (w != root && w != PointerRoot && w != None) {
      /* find top-level window containing current input focus */
      do {
        if (XQueryTree(dpy, (pw = w), &dw, &w, &dws, &du) && dws)
          XFree(dws);
      } while (w != root && w != pw);
      /* find xinerama screen with which the window intersects most */
      if (XGetWindowAttributes(dpy, pw, &wa))
        for (int j = 0; j < n; j++)
          if ((a = INTERSECT(wa.x, wa.y, wa.width, wa.height, info[j])) >
              area) {
            area = a;
            i = j;
          }
    }
    /* no focused window is on screen, so use pointer location instead */
    if (!area && XQueryPointer(dpy, root, &dw, &dw, &x, &y, &di, &di, &du))
      for (i = 0; i < n; i++)
        if (INTERSECT(x, y, 1, 1, info[i]) != 0)
          break;

    sd_out->x = info[i].x_org;
    sd_out->y = info[i].y_org;
    sd_out->width = info[i].width;
    sd_out->height = info[i].height;
    XFree(info);

    return true;
  }
#endif

  return false;
}

namespace sg {

static std::map<uint32_t, XftColor> cachedColors;

XftColor* colorToXftColor(Display *dpy, const Color &color) {
  uint32_t key = color.r + (color.g << 8) + (color.b << 16);
  if (cachedColors.count(key) == 1) {
    return &cachedColors[key];
  }

  // rgb:rr/bb/gg<NULL>
  char buf[4 + 3 + 3 + 2 + 1];
  std::snprintf(buf, sizeof(buf), "rgb:%02x/%02x/%02x", color.r, color.g,
                color.b);

  // FIXME: color should be allocated on screen of rendered window and not default.
  XftColorAllocName(dpy, DefaultVisual(dpy, DefaultScreen(dpy)),
                    DefaultColormap(dpy, DefaultScreen(dpy)), buf, &cachedColors[key]);

  return &cachedColors[key];
}

unsigned int colorToPixel(Display *dpy, const Color &color) {
  return colorToXftColor(dpy, color)->pixel;
}

X11Window::X11Window(Display *dpy, X_Window win)
    // FIXME: drw screen should be screen of rendered window and not default.
    : dpy(dpy), win(win), drw(dpy, DefaultScreen(dpy), win) {
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
  XSetLineAttributes(this->dpy, this->gc, lineWidth, LineSolid, CapNotLast,
                     JoinMiter);
  XDrawRectangle(this->dpy, this->win, this->gc, x, y, width, height);
}

void X11Window::fillRectangle(int x, int y, unsigned int width,
                              unsigned int height, const Color &color) {
  XSetForeground(this->dpy, this->gc, colorToPixel(this->dpy, color));
  XFillRectangle(this->dpy, this->win, this->gc, x, y, width, height);
}

void X11Window::setFonts(const std::vector<Font> &fonts) {
  std::vector<std::string> fontnames;
  fontnames.reserve(fonts.size());

  for (const auto &font : fonts) {
    std::stringstream ss;
    ss << font.family << ":size=" << font.size;
    fontnames.push_back(ss.str());
  }

  this->drw.createAndApplyFontSet(fontnames);
}

void X11Window::drawText(int x, int y, unsigned int width, unsigned int height,
                         const char *text, const Color &color) {
  this->drawText(x, y, width, height, text, color, false);
}

void X11Window::drawText(int x, int y, unsigned int width, unsigned int height,
                         const char *text, const Color &color, bool ellipsis) {
  this->drw.text(x, y, width, height, text, colorToXftColor(this->dpy, color), ellipsis);
}

Dims X11Window::measureText(const char *text) {
  Dims dims;
  dims.height = 0;
  dims.width = this->drw.fontsetGetWidth(text);
  return dims;
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

  DimsAndPos dims = this->getScreenDimensions();

  copied.x = (dims.width - copied.width) / 2 + dims.x;
  copied.y = (dims.height - copied.height) / 2 + dims.y;

  return this->createWindow(copied);
}

DimsAndPos X11Application::getScreenDimensions() {
  DimsAndPos dims;

  if (getXineramaScreenDims(this->dpy, this->root, &dims)) {
    return dims;
  }

  XWindowAttributes attrib;
  XGetWindowAttributes(this->dpy, this->root, &attrib);
  dims.x = 0;
  dims.y = 0;
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
