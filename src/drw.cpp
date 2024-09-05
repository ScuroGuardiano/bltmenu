#include "drw.hpp"
#include "utf8.hpp"
#include <X11/Xft/Xft.h>
#include <cstdio>
#include <utility>

#define LENGTH(X) (sizeof(X) / sizeof(X)[0])

namespace stolen_from_dmenu {

std::optional<Fnt> Fnt::create(Display *dpy, int screen, const char *fontname,
                               FcPattern *fontpattern) {
  XftFont *xfont = NULL;
  FcPattern *pattern = NULL;

  if (fontname) {
    /* Using the pattern found at font->xfont->pattern does not yield the
     * same substitution results as using the pattern returned by
     * FcNameParse; using the latter results in the desired fallback
     * behaviour whereas the former just results in missing-character
     * rectangles being drawn, at least with some fonts. */
    if (!(xfont = XftFontOpenName(dpy, screen, fontname))) {
      fprintf(stderr, "error, cannot load font from name: '%s'\n", fontname);
      return {};
    }
    if (!(pattern = FcNameParse((FcChar8 *)fontname))) {
      fprintf(stderr, "error, cannot parse font name to pattern: '%s'\n",
              fontname);
      XftFontClose(dpy, xfont);
      return {};
    }
  } else if (fontpattern) {
    if (!(xfont = XftFontOpenPattern(dpy, fontpattern))) {
      fprintf(stderr, "error, cannot load font from pattern.\n");
      return {};
    }
  } else {
    fprintf(stderr, "error, no font specified.");
    return {};
  }

  return Fnt(dpy, xfont, pattern);
}

Fnt::~Fnt() {
  if (this->pattern) {
    FcPatternDestroy(this->pattern);
  }
  if (this->xfont) {
    XftFontClose(this->dpy, this->xfont);
  }
}

Fnt::Fnt(Fnt &&other) {
  dpy = std::exchange(other.dpy, nullptr);
  h = std::exchange(other.h, 0);
  xfont = std::exchange(other.xfont, nullptr);
  pattern = std::exchange(other.pattern, nullptr);
}

Fnt &Fnt::operator=(Fnt &&other) {
  dpy = std::exchange(other.dpy, nullptr);
  h = std::exchange(other.h, 0);
  xfont = std::exchange(other.xfont, nullptr);
  pattern = std::exchange(other.pattern, nullptr);
  return *this;
}

void Fnt::getExts(const char* text, unsigned int len, unsigned int *out_w, unsigned int *out_h) {
  XGlyphInfo info;

  if (!text) {
    return;
  }

  XftTextExtentsUtf8(this->dpy, this->xfont, (XftChar8 *)text, len, &info);

  if (out_w) {
    *out_w = info.xOff;
  }
  if (out_h) {
    *out_h = this->h;
  }
}

Fnt::Fnt(Display *dpy, XftFont *xfont, FcPattern *fontpattern)
    : dpy(dpy), h(xfont->ascent + xfont->descent), xfont(xfont),
      pattern(fontpattern) {}

Drw::Drw(Display *dpy, int screen, Drawable drawable)
    : dpy(dpy), screen(screen), drawable(drawable) {}

bool Drw::createAndApplyFontSet(const std::vector<std::string> &fontnames) {
  if (fontnames.empty()) {
    return false;
  }

  std::vector<Fnt> temp;
  temp.reserve(fonts.size());

  for (auto &f : fontnames) {
    auto font = Fnt::create(this->dpy, this->screen, f.c_str(), nullptr);
    if (font) {
      temp.push_back(std::move(*font));
    }
  }

  if (temp.size() > 0) {
    this->fonts = std::move(temp);

    // Moved from original drw_text
  	this->ellipsis_width = this->fontsetGetWidth("...");
  	this->invalid_width = this->fontsetGetWidth(this->invalid);

    return true;
  }

  return false;
}

unsigned int Drw::fontsetGetWidth(const char *text) {
  return this->text(0, 0, 0, 0, text, nullptr, false);
}

/// I won't understand suckless philosophy, for me this function is very hard to read
/// But it works and I have no energy to rewrite it from scratch
/// I just changed it a little bit.
int Drw::text(int x, int y, unsigned int w, unsigned int h, const char *text,
              XftColor *color, bool ellipsis) {
  int ty, ellipsis_x = 0;
  unsigned int tmpw, ew, ellipsis_w = 0, ellipsis_len, hash, h0, h1;
  XftDraw *d = NULL;
  Fnt *usedfont, *nextfont, *curfont;
  std::optional<Fnt> newFont;
  int utf8strlen, render = x || y || w || h;
  CodePoint utf8codepoint;
  const char *utf8str;
  FcCharSet *fccharset;
  FcPattern *fcpattern;
  FcPattern *match;
  XftResult result;
  int charexists = 0, overflow = 0;
  
  // Original drw_text function draws rectangle with background color first
  // Since I want this function to draw text and only text ellipsis wasn't working correctly
  // To fix that I use this variable to track moment from which text can potentially overflow
  // Check the end of the function to see how it's used ^^
  const char *potentialoverlow = nullptr;

  if ((render && (!color || !w)) || !text || this->fonts.empty()) {
    return 0;
  }

  if (!this->fonts[0].pattern) {
    /* Refer to the comment in xfont_create for more information. */
    fprintf(stderr,
            "the first font in the cache must be loaded from a font string.");
    return 0;
  }

  if (!render) {
    // This block is some absurd for me. If we are not rendering anything then
    // we set width depending on invert value? WHAT?! I feel like this is some
    // trolling and since I am not using invert here I will do this and let this
    // abomination remain. btw in original dmenu invert is only used for
    // choosing color, so yeah, funny usage of it.
    int invert = 0;
    w = invert ? invert : ~invert; // <- This is the original line from dmenu
  } else {
    d = XftDrawCreate(this->dpy, this->drawable,
                      DefaultVisual(this->dpy, this->screen),
                      DefaultColormap(this->dpy, this->screen));
  }

  usedfont = &this->fonts[0];

  while (1) {
    ew = ellipsis_len = utf8strlen = 0;
    utf8str = text;
    nextfont = NULL;
    while (*text) {
      utf8codepoint = utf8decode(text);

      auto it = this->fonts.begin();
      for (curfont = it != this->fonts.end() ? &(*it) : nullptr; curfont;
           it++) {
        charexists = charexists || XftCharExists(this->dpy, curfont->xfont,
                                                 utf8codepoint.value);
        if (charexists) {
          curfont->getExts(text, utf8codepoint.len, &tmpw, nullptr);
          if (ew + ellipsis_width <= w) {
            /* keep track where the ellipsis still fits */
            ellipsis_x = x + ew;
            ellipsis_w = w - ew;
            ellipsis_len = utf8strlen;
          }

          if (ew + tmpw + this->ellipsis_width > w && !potentialoverlow && ellipsis) {
            // If writing another character will make less space than ellipsis requires then
            // it's position will be saved. Later if position is saved I won't draw text immediately.
            // It will be drawn only if overflow won't be reached.
            potentialoverlow = text;
          }

          if (ew + tmpw > w) {
            overflow = 1;
            /* called from drw_fontset_getwidth_clamp():
             * it wants the width AFTER the overflow
             */
            if (!render)
              x += tmpw;
            else
              utf8strlen = ellipsis_len;
          } else if (curfont == usedfont) {
            text += utf8codepoint.len;
            utf8strlen += utf8codepoint.err ? 0 : utf8codepoint.len;
            ew += utf8codepoint.err ? 0 : tmpw;
          } else {
            nextfont = curfont;
          }
          break;
        }
      }

      if (overflow || !charexists || nextfont || utf8codepoint.len)
        break;
      else
        charexists = 0;
    }

    if (utf8strlen) {
      if (render && !potentialoverlow) {
        ty = y + (h - usedfont->h) / 2 + usedfont->xfont->ascent;
        XftDrawStringUtf8(d, color, usedfont->xfont, x, ty, (XftChar8 *)utf8str,
                          utf8strlen);
      }
      x += ew;
      w -= ew;
    }
    if (utf8codepoint.err && (!render || invalid_width < w)) {
      if (render) {
        this->text(x, y, w, h, invalid, color, ellipsis);
      }

      x += invalid_width;
      w -= invalid_width;
    }
    if (render && overflow && ellipsis) {
      // drw_text(drw, ellipsis_x, y, ellipsis_w, h, 0, "...", invert);
      this->text(ellipsis_x, y, ellipsis_w, h, "...", color, false);
    }

    if (!*text || overflow) {
      break;
    } else if (nextfont) {
      charexists = 0;
      usedfont = nextfont;
    } else {
      /* Regardless of whether or not a fallback font is found, the
       * character must be drawn. */
      charexists = 1;

      // First font pattern check has been moved to the top of this functon
      // So the function would fail quickly instead failing here

      hash = (unsigned int)utf8codepoint.value;
      hash = ((hash >> 16) ^ hash) * 0x21F0AAAD;
      hash = ((hash >> 15) ^ hash) * 0xD35A2D97;
      h0 = ((hash >> 15) ^ hash) % LENGTH(this->nomatches);
      h1 = (hash >> 17) % LENGTH(this->nomatches);
      /* avoid expensive XftFontMatch call when we know we won't find a match */
      if (this->nomatches[h0] == utf8codepoint.value ||
          this->nomatches[h1] == utf8codepoint.value) {
        goto no_match;
      }

      fccharset = FcCharSetCreate();
      FcCharSetAddChar(fccharset, utf8codepoint.value);

      fcpattern = FcPatternDuplicate(this->fonts[0].pattern);
      FcPatternAddCharSet(fcpattern, FC_CHARSET, fccharset);
      FcPatternAddBool(fcpattern, FC_SCALABLE, FcTrue);

      FcConfigSubstitute(NULL, fcpattern, FcMatchPattern);
      FcDefaultSubstitute(fcpattern);
      match = XftFontMatch(this->dpy, this->screen, fcpattern, &result);

      FcCharSetDestroy(fccharset);
      FcPatternDestroy(fcpattern);

      if (match) {
        newFont = Fnt::create(this->dpy, this->screen, nullptr, match);
        if (newFont &&
            XftCharExists(this->dpy, newFont->xfont, utf8codepoint.value)) {
          this->fonts.push_back(std::move(*newFont));
          newFont = {};
        } else {
          nomatches[nomatches[h0] ? h1 : h0] = utf8codepoint.value;
        no_match:
          usedfont = &this->fonts[0];
        }
      }
    }
  }
  if (d) {
    XftDrawDestroy(d);
  }

  if (potentialoverlow && !overflow) {
    this->text(ellipsis_x, y, ellipsis_width, h, potentialoverlow, color, false);
  }

  return x + (render ? w : 0);
}

} // namespace stolen_from_dmenu
