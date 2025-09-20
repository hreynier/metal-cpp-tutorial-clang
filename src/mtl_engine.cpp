#include "mtl_engine.hpp"
#include "Foundation/NSAutoreleasePool.hpp"
#include "Foundation/NSString.hpp"
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"
#include "Metal/MTLDevice.hpp"
#include "Metal/MTLRenderPipeline.hpp"
#include "Metal/MTLResource.hpp"
#include "vertex_data.hpp"
#include <iostream>
#include <simd/matrix_types.h>
#include <simd/simd.h>

void MTLEngine::init() {
  initDevice();
  initWindow();

  // createTriangle();
  // createSquare();
  createCube();
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

// Function to create a cube, building up from a list of vertices for each face
// etc
void MTLEngine::createCube() {
  // Cube for use in a right-handed coordinate system with triangle faces
  // specified in Counter-Clockwise winding order
  VertexData cubeVertices[] = {
      // Front face
      {{-0.5, -0.5, 0.5, 1.0}, {0.0, 0.0}},
      {{0.5, -0.5, 0.5, 1.0}, {1.0, 0.0}},
      {{0.5, 0.5, 0.5, 1.0}, {1.0, 1.0}},
      {{-0.5, -0.5, 0.5, 1.0}, {1.0, 1.0}},
      {{-0.5, 0.5, 0.5, 1.0}, {0.0, 1.0}},
      {{-0.5, -0.5, 0.5, 1.0}, {0.0, 0.0}},

      // Back face
      {{0.5, -0.5, -0.5, 1.0}, {0.0, 0.0}},
      {{-0.5, -0.5, -0.5, 1.0}, {1.0, 0.0}},
      {{-0.5, 0.5, -0.5, 1.0}, {1.0, 1.0}},
      {{-0.5, 0.5, -0.5, 1.0}, {1.0, 1.0}},
      {{0.5, 0.5, -0.5, 1.0}, {0.0, 1.0}},
      {{0.5, -0.5, -0.5, 1.0}, {0.0, 0.0}},

      // Top face
      {{-0.5, 0.5, 0.5, 1.0}, {0.0, 0.0}},
      {{0.5, 0.5, 0.5, 1.0}, {1.0, 0.0}},
      {{0.5, 0.5, -0.5, 1.0}, {1.0, 1.0}},
      {{0.5, 0.5, -0.5, 1.0}, {1.0, 1.0}},
      {{-0.5, 0.5, -0.5, 1.0}, {0.0, 1.0}},
      {{-0.5, 0.5, 0.5, 1.0}, {0.0, 0.0}},

      // Bottom face
      {{-0.5, -0.5, -0.5, 1.0}, {0.0, 0.0}},
      {{0.5, -0.5, -0.5, 1.0}, {1.0, 0.0}},
      {{0.5, -0.5, 0.5, 1.0}, {1.0, 1.0}},
      {{0.5, -0.5, 0.5, 1.0}, {1.0, 1.0}},
      {{-0.5, -0.5, 0.5, 1.0}, {0.0, 1.0}},
      {{-0.5, -0.5, -0.5, 1.0}, {0.0, 0.0}},

      // Left face
      {{-0.5, -0.5, -0.5, 1.0}, {0.0, 0.0}},
      {{-0.5, -0.5, 0.5, 1.0}, {1.0, 0.0}},
      {{-0.5, 0.5, 0.5, 1.0}, {1.0, 1.0}},
      {{-0.5, 0.5, 0.5, 1.0}, {1.0, 1.0}},
      {{-0.5, 0.5, -0.5, 1.0}, {0.0, 1.0}},
      {{-0.5, -0.5, -0.5, 1.0}, {0.0, 0.0}},

      // Right face
      {{0.5, -0.5, 0.5, 1.0}, {0.0, 0.0}},
      {{0.5, -0.5, -0.5, 1.0}, {1.0, 0.0}},
      {{0.5, 0.5, -0.5, 1.0}, {1.0, 1.0}},
      {{0.5, 0.5, -0.5, 1.0}, {1.0, 1.0}},
      {{0.5, 0.5, 0.5, 1.0}, {0.0, 1.0}},
      {{0.5, -0.5, 0.5, 1.0}, {0.0, 0.0}},
  };

  cubeVertexBuffer = metalDevice->newBuffer(&cubeVertices, sizeof(cubeVertices),
                                            MTL::ResourceStorageModeShared);
  transformationBuffer = metalDevice->newBuffer(sizeof(TransformationData),
                                                MTL::ResourceStorageModeShared);

  grassTexture = new Texture("assets/mc_grass.jpeg", metalDevice);
}

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
      NS::String::string("cubeVertexShader", NS::ASCIIStringEncoding));
  assert(vertexShader);
  MTL::Function *fragmentShader = metalDefaultLibrary->newFunction(
      NS::String::string("cubeFragmentShader", NS::ASCIIStringEncoding));
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

// Define the modal, view, perspective projection's here in the render command
void MTLEngine::encodeRenderCommand(
    MTL::RenderCommandEncoder *renderCommandEncoder) {

  // Moves the cube 2 units down the negative z-axis
  matrix_float4x4 translationMatrix = matrix4x4_translation(0, 0, -1.0);

  // Rotate the cube by 90 degrees
  float angleInDegrees = glfwGetTime() / 2.0 * 45;
  float angleInRadians = angleInDegrees * M_PI / 180.0f;
  matrix_float4x4 rotationMatrix =
      matrix4x4_rotation(angleInRadians, 0.0, 1.0, 0.0);

  matrix_float4x4 modelMatrix = simd_mul(translationMatrix, rotationMatrix);

  simd::float3 R = simd::float3{1, 0, 0};  // Unit-Right
  simd::float3 U = simd::float3{0, 1, 0};  // Unit-Up
  simd::float3 F = simd::float3{0, 0, -1}; // Unit-Forward
  simd::float3 P = simd::float3{0, 0, 1};  // Camera Position in World Space

  matrix_float4x4 viewMatrix = matrix_make_rows(
      R.x, R.y, R.z, simd::dot(-R, P), U.x, U.y, U.z, simd::dot(-U, P), -F.x,
      -F.y, -F.z, simd::dot(F, P), 0, 0, 0, 1);

  // Get the aspect ratio
  // In the future, we could probably do this in the init and store it such that
  // we can control when it's resized ahead of time
  CGSize drawableSize = metalLayer->drawableSize();
  float aspectRatio = (drawableSize.width / drawableSize.height);
  float fov = 90 * (M_PI / 180.0f);
  float nearZ = 0.1f;
  float farZ = 100.0f;

  matrix_float4x4 perspectiveMatrix =
      matrix_perspective_right_hand(fov, aspectRatio, nearZ, farZ);

  TransformationData transformationData = {modelMatrix, viewMatrix,
                                           perspectiveMatrix};
  memcpy(transformationBuffer->contents(), &transformationData,
         sizeof(transformationData));

  renderCommandEncoder->setRenderPipelineState(metalRenderPS0);
  renderCommandEncoder->setVertexBuffer(cubeVertexBuffer, 0, 0);
  renderCommandEncoder->setVertexBuffer(transformationBuffer, 0, 1);
  MTL::PrimitiveType typeTriangle = MTL::PrimitiveTypeTriangle;
  NS::UInteger vertexStart = 0;
  NS::UInteger vertexCount = 36;
  renderCommandEncoder->setFragmentTexture(grassTexture->texture, 0);
  renderCommandEncoder->drawPrimitives(typeTriangle, vertexStart, vertexCount);
};
