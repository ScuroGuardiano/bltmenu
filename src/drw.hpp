/// I stole drw from dmenu but... you guess it! Bloated it.
#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>
#include <optional>
#include <string>
#include <vector>

namespace stolen_from_dmenu {

struct Fnt {
  static std::optional<Fnt> create(Display *dpy, int screen,
                                   const char *fontname,
                                   FcPattern *fontpattern);
  ~Fnt();

  Fnt(const Fnt &other) = delete;
  Fnt &operator=(const Fnt &other) = delete;
  Fnt(Fnt &&other);
  Fnt &operator=(Fnt &&other);

  void getExts(const char* text, unsigned int len, unsigned int *out_w, unsigned int *out_h);

  Display *dpy;
  unsigned int h;
  XftFont *xfont;
  FcPattern *pattern;

private:
  Fnt(Display *dpy, XftFont *xfont, FcPattern *fontpattern);
};

/// In dmenu drw is used to draw everything
/// I use it only for drawing text, because it's quite damn hard
/// I changed implementation from drw.c, commenting some changes in the source file
class Drw {
public:
  Drw(Display* dpy, int screen, Drawable drawable);
  bool createAndApplyFontSet(const std::vector<std::string> &fontnames);
  unsigned int fontsetGetWidth(const char *text);
  int text(int x, int y, unsigned int w, unsigned int h, const char* text, XftColor* color, bool ellipsis);

private:
  Display *dpy;
  int screen;
  Drawable drawable;
  unsigned int ellipsis_width = 0;
  unsigned int invalid_width = 0;
  unsigned int nomatches[128];
  const char* invalid = "�";
  std::vector<Fnt> fonts;
};

} // namespace stolen_from_dmenu
