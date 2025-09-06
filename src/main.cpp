// #include <iostream>
// #define GLFW_INCLUDE_NONE
// #include <GLFW/glfw3.h>
// #include <Metal/Metal.hpp>
#include "mtl_engine.hpp"
#include "mtl_implementation.cpp"

int main() {
  MTLEngine engine;
  engine.init();
  engine.run();
  engine.cleanup();

  return 0;
}
