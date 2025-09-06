#include "texture.hpp"

Texture::Texture(const char *filepath, MTL::Device *metalDevice) {
  device = metalDevice;

  // Metal expects 0 coordinate to be on the bottom of image rather than top
  stbi_set_flip_vertically_on_load(true);

  // Load image, asset that it's not null
  unsigned char *image =
      stbi_load(filepath, &width, &height, &channels, STBI_rgb_alpha);
  assert(image != NULL);

  // Create a texture descriptor that specifies texture properties
  // (format, dimensions, etc)
  MTL::TextureDescriptor *textureDescriptor =
      MTL::TextureDescriptor::alloc()->init();
  textureDescriptor->setPixelFormat(
      MTL::PixelFormatRGBA8Unorm); // 8-bit RGBA format
  textureDescriptor->setWidth(width);
  textureDescriptor->setHeight(height);

  // Create empty GPU Texture using descriptor
  texture = device->newTexture(textureDescriptor);

  // Create a 3D region starting at origin (0,0,0), with dimensions
  // (width*height*1)
  MTL::Region region = MTL::Region(0, 0, 0, width, height, 1);
  // Calculate how many bytes per each row of pixels
  // In this case, RGBA == 4 bytes, one per channel
  NS::UInteger bytesPerRow = 4 * width;
  // Copy the image data from the CPU memory (image) into GPU Texture memory
  // Region specifies the part of the texture to update, 0 -> mipmap level (0 ==
  // full res) image -> pointer to image data in CPU bytesPerRow -> stride
  // informatiion for data layout
  texture->replaceRegion(region, 0, image, bytesPerRow);

  // Release the texture descriptor object from memory
  textureDescriptor->release();
  // Free CPU image data since it's now on the GPU
  stbi_image_free(image);
};

Texture::~Texture() { texture->release(); }
