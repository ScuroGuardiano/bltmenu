#include "utf8.hpp"

namespace stolen_from_dmenu {

CodePoint utf8decode(const char *s_in) {
  static const unsigned char lens[] = {
      /* 0XXXX */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      /* 10XXX */ 0, 0, 0, 0, 0, 0, 0, 0, /* invalid */
      /* 110XX */ 2, 2, 2, 2,
      /* 1110X */ 3, 3,
      /* 11110 */ 4,
      /* 11111 */ 0, /* invalid */
  };
  static const unsigned char leading_mask[] = {0x7F, 0x1F, 0x0F, 0x07};
  static const unsigned int overlong[] = {0x0, 0x80, 0x0800, 0x10000};

  CodePoint ret;
  ret.ref = s_in;

  const unsigned char *s = (const unsigned char *)s_in;
  int len = lens[*s >> 3];
  ret.value = UTF_INVALID;
  ret.err = 1;

  if (len == 0) {
    ret.err = true;
    return ret;
  }

  long cp = s[0] & leading_mask[len - 1];
  for (int i = 1; i < len; ++i) {
    if (s[i] == '\0' || (s[i] & 0xC0) != 0x80) {
      ret.len = i;
      return ret;
    }
    cp = (cp << 6) | (s[i] & 0x3F);
  }
  /* out of range, surrogate, overlong encoding */
  if (cp > 0x10FFFF || (cp >> 11) == 0x1B || cp < overlong[len - 1]) {
    ret.len = len;
    return ret;
  }

  ret.err = false;
  ret.value = cp;
  ret.len = len;

  return ret;
}

} // namespace sg
