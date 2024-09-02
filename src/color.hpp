#pragma once

#include <cstdint>
namespace sg {

struct Color {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

inline Color rgb(uint8_t r, uint8_t g, uint8_t b) {
  return Color { r, g, b };
}

}
