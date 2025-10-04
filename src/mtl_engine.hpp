#pragma once

#include "Metal/MTLRenderCommandEncoder.hpp"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>

#include "texture.hpp"
#include "tiny_obj_loader.h"
#include "vertex_data.hpp"
#include <stb/stb_image.h>

#include <AAPLMathUtilities.h>
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
  void createSphere(int numLat = 34, int numLon = 34);
  void loadObjModel(const char *filename);
  void createLight();
  void createTriangle();
  void createCube();
  void createBuffers();
  void createDefaultLibrary();
  void createCommandQueue();
  void createRenderPipeline();
  void createLightSourceRenderPipeline();
  void createDepthAndMSAATextures();
  void createRenderPassDescriptor();

  // Upon resizing, update depth and MSAA
  void updateRenderPassDescriptor();

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
  MTL::RenderPipelineState *metalLightSourceRenderPSO;

  MTL::DepthStencilState *depthStencilState;
  MTL::RenderPassDescriptor *renderPassDescriptor;
  MTL::Texture *msaaRenderTargetTexture = nullptr;
  int sampleCount = 4;
  MTL::Texture *depthTexture;

  NS::UInteger vertexCount = 0;

  MTL::Buffer *squareVertexBuffer;
  MTL::Buffer *sphereVertexBuffer;
  MTL::Buffer *sphereTransformationBuffer;
  MTL::Buffer *objVertexBuffer = nullptr;
  MTL::Buffer *objTransformationBuffer = nullptr;
  MTL::Buffer *lightVertexBuffer;
  MTL::Buffer *lightTransformationBuffer;
  MTL::Buffer *triangleVertexBuffer;
  MTL::Buffer *cubeVertexBuffer;
  MTL::Buffer *transformationBuffer;

  Texture *grassTexture;
  Texture *marsTexture = nullptr;
};
