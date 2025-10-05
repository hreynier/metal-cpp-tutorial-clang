#pragma once
#include "stb_image.h"
#include <Metal/Metal.hpp>

class Texture {
public:
  Texture(const char *filepath, MTL::Device *metalDevice);
  ~Texture();
  MTL::Texture *texture;
  int width, height, channels;

private:
  MTL::Device *device;
};
