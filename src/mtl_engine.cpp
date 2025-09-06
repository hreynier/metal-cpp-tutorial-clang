#include "mtl_engine.hpp"
#include "Foundation/NSAutoreleasePool.hpp"
#include "Foundation/NSString.hpp"
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"
#include "Metal/MTLDevice.hpp"
#include "Metal/MTLRenderPipeline.hpp"
#include "Metal/MTLResource.hpp"
#include <fstream>
#include <iostream>
#include <simd/simd.h>
#include <string>

void MTLEngine::init() {
  initDevice();
  initWindow();

  // createTriangle();
  createSquare();
  createDefaultLibrary();
  createCommandQueue();
  createRenderPipeline();
};

void MTLEngine::run() {
  while (!glfwWindowShouldClose(window)) {
    NS::AutoreleasePool *pool = NS::AutoreleasePool::alloc()->init();
    metalDrawable = metalLayer->nextDrawable();
    draw();
    pool->release();
    glfwPollEvents();
  }
};

void MTLEngine::cleanup() {
  glfwTerminate();
  metalDevice->release();
};

void MTLEngine::initDevice() { metalDevice = MTL::CreateSystemDefaultDevice(); }

void MTLEngine::frameBufferSizeCallback(GLFWwindow *window, int width,
                                        int height) {
  MTLEngine *engine = (MTLEngine *)glfwGetWindowUserPointer(window);
  engine->resizeFrameBuffer(width, height);
};

void MTLEngine::resizeFrameBuffer(int width, int height) {
  metalLayer->setDrawableSize(CGSizeMake(width, height));
};

void MTLEngine::initWindow() {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window = glfwCreateWindow(800, 600, "Metal Engine", NULL, NULL);
  if (!window) {
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  glfwSetWindowUserPointer(window, this);
  glfwSetFramebufferSizeCallback(window, frameBufferSizeCallback);
  int width, height;
  glfwGetFramebufferSize(window, &width, &height);

  metalWindow = reinterpret_cast<NS::Window *>(glfwGetCocoaWindow(window));
  metalLayer = CA::MetalLayer::layer();
  metalLayer->setDevice(metalDevice);
  metalLayer->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
  metalLayer->setDrawableSize(CGSizeMake(width, height));
  NS::View *nsview = metalWindow->contentView();
  nsview->setLayer(metalLayer);
  nsview->setWantsLayer(true);
};

// std::string MTLEngine::readFile(const std::string &filename) {
//   std::ifstream file(filename);
//   if (!file.is_open()) {
//     throw std::runtime_error("Could not open file: " + filename);
//   }
//   return std::string((std::istreambuf_iterator<char>(file)),
//                      std::istreambuf_iterator<char>());
// };

void MTLEngine::createSquare() {
  // We have six vertices to define the square
  // This is because our square is made up of two triangles, sharing two
  // vertices at the corners.
  VertexData squareVertices[] = {
      {{-0.5, -0.5, 0.5, 1.0f}, {0.0f, 0.0f}},
      {{-0.5, 0.5, 0.5, 1.0f}, {0.0f, 1.0f}},
      {{0.5, 0.5, 0.5, 1.0f}, {1.0f, 1.0f}},
      {{-0.5, -0.5, 0.5, 1.0f}, {0.0f, 0.0f}},
      {{0.5, 0.5, 0.5, 1.0f}, {1.0f, 1.0f}},
      {{0.5, -0.5, 0.5, 1.0f}, {1.0f, 0.0f}},
  };

  // Create a buffer for the vertices, defining the size in memory for the GPU
  squareVertexBuffer = metalDevice->newBuffer(
      &squareVertices, sizeof(squareVertices), MTL::ResourceStorageModeShared);

  // Create a grass texture object, pointing to relative file location
  grassTexture = new Texture("assets/mc_grass.jpeg", metalDevice);
};

void MTLEngine::createTriangle() {
  simd::float3 triangleVertices[] = {
      {-0.5f, -0.5f, 0.0f}, {0.5f, -0.5f, 0.0f}, {0.0f, 0.5f, 0.0f}};

  triangleVertexBuffer =
      metalDevice->newBuffer(&triangleVertices, sizeof(triangleVertices),
                             MTL::ResourceStorageModeShared);
};

void MTLEngine::createDefaultLibrary() {
  // Load the precompiled metallib file
  NS::String *libraryPath =
      NS::String::string("shaders.metallib", NS::UTF8StringEncoding);
  NS::Error *error = nullptr;

  // Try to load from the current working directory (where CMake copies it)
  metalDefaultLibrary = metalDevice->newLibrary(libraryPath, &error);

  if (!metalDefaultLibrary) {
    std::cerr << "Failed to load metal library: " << libraryPath->utf8String()
              << std::endl;
    if (error) {
      std::cerr << "Error: " << error->localizedDescription()->utf8String()
                << std::endl;
    }
    std::exit(-1);
  }

  std::cout << "Successfully loaded Metal library: "
            << libraryPath->utf8String() << std::endl;
};

void MTLEngine::createCommandQueue() {
  metalCommandQueue = metalDevice->newCommandQueue();
};

void MTLEngine::createRenderPipeline() {
  MTL::Function *vertexShader = metalDefaultLibrary->newFunction(
      NS::String::string("squareVertexShader", NS::ASCIIStringEncoding));
  assert(vertexShader);
  MTL::Function *fragmentShader = metalDefaultLibrary->newFunction(
      NS::String::string("squareFragmentShader", NS::ASCIIStringEncoding));
  assert(fragmentShader);

  MTL::RenderPipelineDescriptor *renderPipelineDescriptor =
      MTL::RenderPipelineDescriptor::alloc()->init();
  renderPipelineDescriptor->setLabel(NS::String::string(
      "Triangle Rendering Pipeline", NS::ASCIIStringEncoding));
  renderPipelineDescriptor->setVertexFunction(vertexShader);
  renderPipelineDescriptor->setFragmentFunction(fragmentShader);
  assert(renderPipelineDescriptor);

  MTL::PixelFormat pixelFormat = metalLayer->pixelFormat();
  renderPipelineDescriptor->colorAttachments()->object(0)->setPixelFormat(
      pixelFormat);

  NS::Error *error;
  metalRenderPS0 =
      metalDevice->newRenderPipelineState(renderPipelineDescriptor, &error);
  renderPipelineDescriptor->release();
  vertexShader->release();
  fragmentShader->release();
};

void MTLEngine::draw() { sendRenderCommand(); };

void MTLEngine::sendRenderCommand() {
  metalCommandBuffer = metalCommandQueue->commandBuffer();

  MTL::RenderPassDescriptor *renderPassDescriptor =
      MTL::RenderPassDescriptor::alloc()->init();
  MTL::RenderPassColorAttachmentDescriptor *cd =
      renderPassDescriptor->colorAttachments()->object(0);
  cd->setTexture(metalDrawable->texture());
  cd->setLoadAction(MTL::LoadActionClear);
  cd->setClearColor(
      MTL::ClearColor(41.0f / 255.0f, 42.0f / 255.0f, 48.0f / 255.0f, 1.0f));
  cd->setStoreAction(MTL::StoreActionStore);

  MTL::RenderCommandEncoder *renderCommandEncoder =
      metalCommandBuffer->renderCommandEncoder(renderPassDescriptor);
  encodeRenderCommand(renderCommandEncoder);
  renderCommandEncoder->endEncoding();

  metalCommandBuffer->presentDrawable(metalDrawable);
  metalCommandBuffer->commit();
  metalCommandBuffer->waitUntilCompleted();

  renderPassDescriptor->release();
};

void MTLEngine::encodeRenderCommand(
    MTL::RenderCommandEncoder *renderCommandEncoder) {
  renderCommandEncoder->setRenderPipelineState(metalRenderPS0);
  // Here we specify the buffer to load in the vertex shader for the square or
  // triangle. renderCommandEncoder->setVertexBuffer(triangleVertexBuffer, 0,
  // 0);
  renderCommandEncoder->setVertexBuffer(squareVertexBuffer, 0, 0);
  MTL::PrimitiveType typeTriangle = MTL::PrimitiveTypeTriangle;
  NS::UInteger vertexStart = 0;
  // As we have 6 vertices for a square, we have to add that here
  // NS::UInteger vertexEnd = 3;
  NS::UInteger vertexEnd = 6;
  // Finally, we have to set the texture to use in our fragment shader
  renderCommandEncoder->setFragmentTexture(grassTexture->texture, 0);
  renderCommandEncoder->drawPrimitives(typeTriangle, vertexStart, vertexEnd);
};
