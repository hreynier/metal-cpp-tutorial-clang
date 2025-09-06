#pragma once

#include "Metal/MTLRenderCommandEncoder.hpp"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>

#include "texture.hpp"
#include "vertex_data.hpp"
#include <stb/stb_image.h>

#include <AppKit/AppKit.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/CAMetalLayer.hpp>
#include <QuartzCore/QuartzCore.hpp>

#include <filesystem>

class MTLEngine {
public:
  void init();
  void run();
  void cleanup();

private:
  void initDevice();
  void initWindow();
  static void frameBufferSizeCallback(GLFWwindow *window, int width,
                                      int height);
  void resizeFrameBuffer(int width, int height);
  std::string readFile(const std::string &filename);

  void createSquare();
  void createTriangle();
  void createDefaultLibrary();
  void createCommandQueue();
  void createRenderPipeline();

  void encodeRenderCommand(MTL::RenderCommandEncoder *renderEncoder);
  void sendRenderCommand();
  void draw();

  MTL::Device *metalDevice;
  GLFWwindow *window;
  NS::Window *metalWindow;
  CA::MetalLayer *metalLayer;
  CA::MetalDrawable *metalDrawable;

  MTL::Library *metalDefaultLibrary;
  MTL::CommandQueue *metalCommandQueue;
  MTL::CommandBuffer *metalCommandBuffer;
  MTL::RenderPipelineState *metalRenderPS0;
  MTL::Buffer *squareVertexBuffer;
  MTL::Buffer *triangleVertexBuffer;

  Texture *grassTexture;
};
