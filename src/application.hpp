#pragma once

#include "window.hpp"
#include <memory>

namespace sg {

class Application {

public:
  virtual std::shared_ptr<Window> createWindow(const WindowInit& init) = 0;
  virtual std::shared_ptr<Window> createCenteredWindow(const WindowInit& init) = 0;
  virtual WindowDims getRootDimensions() = 0;
  virtual void run() = 0;

  virtual ~Application() {};
};

}
