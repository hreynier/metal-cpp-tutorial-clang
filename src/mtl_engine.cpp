#include "mtl_engine.hpp"
#include "Foundation/NSAutoreleasePool.hpp"
#include "Foundation/NSString.hpp"
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"
#include "Metal/MTLDepthStencil.hpp"
#include "Metal/MTLDevice.hpp"
#include "Metal/MTLPixelFormat.hpp"
#include "Metal/MTLRenderCommandEncoder.hpp"
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
  // createCube();
  createSphere();
  createLight();
  createBuffers();
  createDefaultLibrary();
  createCommandQueue();
  createRenderPipeline();
  createLightSourceRenderPipeline();
  createDepthAndMSAATextures();
  createRenderPassDescriptor();
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
  sphereTransformationBuffer->release();
  lightTransformationBuffer->release();
  msaaRenderTargetTexture->release();
  depthTexture->release();
  renderPassDescriptor->release();
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

  // Deallocate created textures
  if (msaaRenderTargetTexture) {
    msaaRenderTargetTexture->release();
    msaaRenderTargetTexture = nullptr;
  }
  if (depthTexture) {
    depthTexture->release();
    depthTexture = nullptr;
  }
  createDepthAndMSAATextures();
  metalDrawable = metalLayer->nextDrawable();
  updateRenderPassDescriptor();
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
  metalDrawable = metalLayer->nextDrawable();
};

void MTLEngine::createSphere(int numLat, int numLong) {
  // To create a sphere, we need to compose it of squares
  std::vector<VertexData> vertices;
  const float PI = 3.14159265359f;

  // For each lat/lon we need to create a square
  for (int lat = 0; lat < numLat; lat++) {
    for (int lon = 0; lon < numLong; lon++) {
      // Corners of square
      std::array<simd::float4, 4> squareVertices;
      std::array<simd::float4, 4> normals;
      std::array<simd::float2, 4> texCoords;

      for (int i = 0; i < 4; i++) {
        float theta = (lat + (i / 2)) * PI / numLat;    // 0 to Pi
        float phi = (lon + (i % 2)) * 2 * PI / numLong; // 0 to 2 Pi
        float sinTheta = sinf(theta);
        float cosTheta = cosf(theta);
        float sinPhi = sinf(phi);
        float cosPhi = cosf(phi);

        squareVertices[i] = {cosPhi * sinTheta, cosTheta, sinPhi * sinTheta,
                             1.0f};
        // Normal of the vertex is same as its position on a unit sphere.
        normals[i] = simd::normalize(squareVertices[i]);

        // Texture coordinate in spherical coordinates
        float u = phi / (2 * PI);
        float v = theta / PI;
        texCoords[i] = {u, v};
      }

      // Create two triangles to form the square in counter clockwise winding
      // order
      // First Triangle
      vertices.push_back(
          VertexData{squareVertices[0], texCoords[0], normals[0]});
      vertices.push_back(
          VertexData{squareVertices[1], texCoords[1], normals[1]});
      vertices.push_back(
          VertexData{squareVertices[2], texCoords[2], normals[2]});
      // Second Triangle
      vertices.push_back(
          VertexData{squareVertices[1], texCoords[1], normals[1]});
      vertices.push_back(
          VertexData{squareVertices[3], texCoords[3], normals[3]});
      vertices.push_back(
          VertexData{squareVertices[2], texCoords[2], normals[2]});
    }
  }
  sphereVertexBuffer = metalDevice->newBuffer(
      vertices.data(), sizeof(VertexData) * vertices.size(),
      MTL::ResourceStorageModeShared);

  vertexCount = vertices.size();
  marsTexture = new Texture("assets/mars_texture.jpg", metalDevice);
}

void MTLEngine::createLight() {
  // Cube for use in right-handed coord system with triangle faces
  // specified with counter-clockwise winding order
  VertexData lightSource[] = {
      // Front face (normal: 0, 0, 1)
      {{-0.5, -0.5, 0.5, 1.0}, {0.0, 0.0}, {0.0, 0.0, 1.0, 1.0}},
      {{0.5, -0.5, 0.5, 1.0}, {0.0, 0.0}, {0.0, 0.0, 1.0, 1.0}},
      {{0.5, 0.5, 0.5, 1.0}, {0.0, 0.0}, {0.0, 0.0, 1.0, 1.0}},
      {{-0.5, -0.5, 0.5, 1.0}, {0.0, 0.0}, {0.0, 0.0, 1.0, 1.0}},
      {{0.5, 0.5, 0.5, 1.0}, {0.0, 0.0}, {0.0, 0.0, 1.0, 1.0}},
      {{-0.5, 0.5, 0.5, 1.0}, {0.0, 0.0}, {0.0, 0.0, 1.0, 1.0}},

      // Back face (normal: 0, 0, -1)
      {{0.5, -0.5, -0.5, 1.0}, {0.0, 0.0}, {0.0, 0.0, -1.0, 1.0}},
      {{-0.5, -0.5, -0.5, 1.0}, {0.0, 0.0}, {0.0, 0.0, -1.0, 1.0}},
      {{-0.5, 0.5, -0.5, 1.0}, {0.0, 0.0}, {0.0, 0.0, -1.0, 1.0}},
      {{-0.5, 0.5, -0.5, 1.0}, {0.0, 0.0}, {0.0, 0.0, -1.0, 1.0}},
      {{0.5, 0.5, -0.5, 1.0}, {0.0, 0.0}, {0.0, 0.0, -1.0, 1.0}},
      {{0.5, -0.5, -0.5, 1.0}, {0.0, 0.0}, {0.0, 0.0, -1.0, 1.0}},

      // Top face (normal: 0, 1, 0)
      {{-0.5, 0.5, 0.5, 1.0}, {0.0, 0.0}, {0.0, 1.0, 0.0, 1.0}},
      {{0.5, 0.5, 0.5, 1.0}, {0.0, 0.0}, {0.0, 1.0, 0.0, 1.0}},
      {{0.5, 0.5, -0.5, 1.0}, {0.0, 0.0}, {0.0, 1.0, 0.0, 1.0}},
      {{0.5, 0.5, -0.5, 1.0}, {0.0, 0.0}, {0.0, 1.0, 0.0, 1.0}},
      {{-0.5, 0.5, -0.5, 1.0}, {0.0, 0.0}, {0.0, 1.0, 0.0, 1.0}},
      {{-0.5, 0.5, 0.5, 1.0}, {0.0, 0.0}, {0.0, 1.0, 0.0, 1.0}},

      // Bottom face (normal: 0, -1, 0)
      {{-0.5, -0.5, -0.5, 1.0}, {0.0, 0.0}, {0.0, -1.0, 0.0, 1.0}},
      {{0.5, -0.5, -0.5, 1.0}, {0.0, 0.0}, {0.0, -1.0, 0.0, 1.0}},
      {{0.5, -0.5, 0.5, 1.0}, {0.0, 0.0}, {0.0, -1.0, 0.0, 1.0}},
      {{0.5, -0.5, 0.5, 1.0}, {0.0, 0.0}, {0.0, -1.0, 0.0, 1.0}},
      {{-0.5, -0.5, 0.5, 1.0}, {0.0, 0.0}, {0.0, -1.0, 0.0, 1.0}},
      {{-0.5, -0.5, -0.5, 1.0}, {0.0, 0.0}, {0.0, -1.0, 0.0, 1.0}},

      // Left face (normal: -1, 0, 0)
      {{-0.5, -0.5, -0.5, 1.0}, {0.0, 0.0}, {-1.0, 0.0, 0.0, 1.0}},
      {{-0.5, -0.5, 0.5, 1.0}, {0.0, 0.0}, {-1.0, 0.0, 0.0, 1.0}},
      {{-0.5, 0.5, 0.5, 1.0}, {0.0, 0.0}, {-1.0, 0.0, 0.0, 1.0}},
      {{-0.5, 0.5, 0.5, 1.0}, {0.0, 0.0}, {-1.0, 0.0, 0.0, 1.0}},
      {{-0.5, 0.5, -0.5, 1.0}, {0.0, 0.0}, {-1.0, 0.0, 0.0, 1.0}},
      {{-0.5, -0.5, -0.5, 1.0}, {0.0, 0.0}, {-1.0, 0.0, 0.0, 1.0}},

      // Right face (normal: 1, 0, 0)
      {{0.5, -0.5, 0.5, 1.0}, {0.0, 0.0}, {1.0, 0.0, 0.0, 1.0}},
      {{0.5, -0.5, -0.5, 1.0}, {0.0, 0.0}, {1.0, 0.0, 0.0, 1.0}},
      {{0.5, 0.5, -0.5, 1.0}, {0.0, 0.0}, {1.0, 0.0, 0.0, 1.0}},
      {{0.5, 0.5, -0.5, 1.0}, {0.0, 0.0}, {1.0, 0.0, 0.0, 1.0}},
      {{0.5, 0.5, 0.5, 1.0}, {0.0, 0.0}, {1.0, 0.0, 0.0, 1.0}},
      {{0.5, -0.5, 0.5, 1.0}, {0.0, 0.0}, {1.0, 0.0, 0.0, 1.0}},
  };

  lightVertexBuffer = metalDevice->newBuffer(lightSource, sizeof(lightSource),
                                             MTL::ResourceStorageModeShared);
}

// void MTLEngine::createSquare() {
//   // We have six vertices to define the square
//   // This is because our square is made up of two triangles, sharing two
//   // vertices at the corners.
//   VertexData squareVertices[] = {
//       {{-0.5, -0.5, 0.5, 1.0f}, {0.0f, 0.0f}},
//       {{-0.5, 0.5, 0.5, 1.0f}, {0.0f, 1.0f}},
//       {{0.5, 0.5, 0.5, 1.0f}, {1.0f, 1.0f}},
//       {{-0.5, -0.5, 0.5, 1.0f}, {0.0f, 0.0f}},
//       {{0.5, 0.5, 0.5, 1.0f}, {1.0f, 1.0f}},
//       {{0.5, -0.5, 0.5, 1.0f}, {1.0f, 0.0f}},
//   };

//   // Create a buffer for the vertices, defining the size in memory for the
//   GPU squareVertexBuffer = metalDevice->newBuffer(
//       &squareVertices, sizeof(squareVertices),
//       MTL::ResourceStorageModeShared);

//   // Create a grass texture object, pointing to relative file location
//   grassTexture = new Texture("assets/mc_grass.jpeg", metalDevice);
// };

// void MTLEngine::createTriangle() {
//   simd::float3 triangleVertices[] = {
//       {-0.5f, -0.5f, 0.0f}, {0.5f, -0.5f, 0.0f}, {0.0f, 0.5f, 0.0f}};

//   triangleVertexBuffer =
//       metalDevice->newBuffer(&triangleVertices, sizeof(triangleVertices),
//                              MTL::ResourceStorageModeShared);
// };

// // Function to create a cube, building up from a list of vertices for each
// face
// // etc
// void MTLEngine::createCube() {
//   // Cube for use in a right-handed coordinate system with triangle faces
//   // specified in Counter-Clockwise winding order
//   VertexData cubeVertices[] = {
//       // Front face (normal: 0, 0, 1)
//       {{-0.5, -0.5, 0.5, 1.0}, {0.0, 0.0}, {0.0, 0.0, 1.0}},
//       {{0.5, -0.5, 0.5, 1.0}, {1.0, 0.0}, {0.0, 0.0, 1.0}},
//       {{0.5, 0.5, 0.5, 1.0}, {1.0, 1.0}, {0.0, 0.0, 1.0}},
//       {{-0.5, -0.5, 0.5, 1.0}, {0.0, 0.0}, {0.0, 0.0, 1.0}},
//       {{0.5, 0.5, 0.5, 1.0}, {1.0, 1.0}, {0.0, 0.0, 1.0}},
//       {{-0.5, 0.5, 0.5, 1.0}, {0.0, 1.0}, {0.0, 0.0, 1.0}},

//       // Back face (normal: 0, 0, -1)
//       {{0.5, -0.5, -0.5, 1.0}, {0.0, 0.0}, {0.0, 0.0, -1.0}},
//       {{-0.5, -0.5, -0.5, 1.0}, {1.0, 0.0}, {0.0, 0.0, -1.0}},
//       {{-0.5, 0.5, -0.5, 1.0}, {1.0, 1.0}, {0.0, 0.0, -1.0}},
//       {{-0.5, 0.5, -0.5, 1.0}, {1.0, 1.0}, {0.0, 0.0, -1.0}},
//       {{0.5, 0.5, -0.5, 1.0}, {0.0, 1.0}, {0.0, 0.0, -1.0}},
//       {{0.5, -0.5, -0.5, 1.0}, {0.0, 0.0}, {0.0, 0.0, -1.0}},

//       // Top face (normal: 0, 1, 0)
//       {{-0.5, 0.5, 0.5, 1.0}, {0.0, 0.0}, {0.0, 1.0, 0.0}},
//       {{0.5, 0.5, 0.5, 1.0}, {1.0, 0.0}, {0.0, 1.0, 0.0}},
//       {{0.5, 0.5, -0.5, 1.0}, {1.0, 1.0}, {0.0, 1.0, 0.0}},
//       {{0.5, 0.5, -0.5, 1.0}, {1.0, 1.0}, {0.0, 1.0, 0.0}},
//       {{-0.5, 0.5, -0.5, 1.0}, {0.0, 1.0}, {0.0, 1.0, 0.0}},
//       {{-0.5, 0.5, 0.5, 1.0}, {0.0, 0.0}, {0.0, 1.0, 0.0}},

//       // Bottom face (normal: 0, -1, 0)
//       {{-0.5, -0.5, -0.5, 1.0}, {0.0, 0.0}, {0.0, -1.0, 0.0}},
//       {{0.5, -0.5, -0.5, 1.0}, {1.0, 0.0}, {0.0, -1.0, 0.0}},
//       {{0.5, -0.5, 0.5, 1.0}, {1.0, 1.0}, {0.0, -1.0, 0.0}},
//       {{0.5, -0.5, 0.5, 1.0}, {1.0, 1.0}, {0.0, -1.0, 0.0}},
//       {{-0.5, -0.5, 0.5, 1.0}, {0.0, 1.0}, {0.0, -1.0, 0.0}},
//       {{-0.5, -0.5, -0.5, 1.0}, {0.0, 0.0}, {0.0, -1.0, 0.0}},

//       // Left face (normal: -1, 0, 0)
//       {{-0.5, -0.5, -0.5, 1.0}, {0.0, 0.0}, {-1.0, 0.0, 0.0}},
//       {{-0.5, -0.5, 0.5, 1.0}, {1.0, 0.0}, {-1.0, 0.0, 0.0}},
//       {{-0.5, 0.5, 0.5, 1.0}, {1.0, 1.0}, {-1.0, 0.0, 0.0}},
//       {{-0.5, 0.5, 0.5, 1.0}, {1.0, 1.0}, {-1.0, 0.0, 0.0}},
//       {{-0.5, 0.5, -0.5, 1.0}, {0.0, 1.0}, {-1.0, 0.0, 0.0}},
//       {{-0.5, -0.5, -0.5, 1.0}, {0.0, 0.0}, {-1.0, 0.0, 0.0}},

//       // Right face (normal: 1, 0, 0)
//       {{0.5, -0.5, 0.5, 1.0}, {0.0, 0.0}, {1.0, 0.0, 0.0}},
//       {{0.5, -0.5, -0.5, 1.0}, {1.0, 0.0}, {1.0, 0.0, 0.0}},
//       {{0.5, 0.5, -0.5, 1.0}, {1.0, 1.0}, {1.0, 0.0, 0.0}},
//       {{0.5, 0.5, -0.5, 1.0}, {1.0, 1.0}, {1.0, 0.0, 0.0}},
//       {{0.5, 0.5, 0.5, 1.0}, {0.0, 1.0}, {1.0, 0.0, 0.0}},
//       {{0.5, -0.5, 0.5, 1.0}, {0.0, 0.0}, {1.0, 0.0, 0.0}},
//   };

//   cubeVertexBuffer = metalDevice->newBuffer(&cubeVertices,
//   sizeof(cubeVertices),
//                                             MTL::ResourceStorageModeShared);

//   grassTexture = new Texture("assets/mc_grass.jpeg", metalDevice);
// }

void MTLEngine::createBuffers() {
  // transformationBuffer = metalDevice->newBuffer(sizeof(TransformationData),
  // MTL::ResourceStorageModeShared);
  sphereTransformationBuffer = metalDevice->newBuffer(
      sizeof(TransformationData), MTL::ResourceStorageModeShared);

  lightTransformationBuffer = metalDevice->newBuffer(
      sizeof(TransformationData), MTL::ResourceStorageModeShared);
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
      NS::String::string("sphereVertexShader", NS::ASCIIStringEncoding));
  assert(vertexShader);
  MTL::Function *fragmentShader = metalDefaultLibrary->newFunction(
      NS::String::string("sphereFragmentShader", NS::ASCIIStringEncoding));
  assert(fragmentShader);

  MTL::RenderPipelineDescriptor *renderPipelineDescriptor =
      MTL::RenderPipelineDescriptor::alloc()->init();
  renderPipelineDescriptor->setLabel(
      NS::String::string("Sphere Rendering Pipeline", NS::ASCIIStringEncoding));
  renderPipelineDescriptor->setVertexFunction(vertexShader);
  renderPipelineDescriptor->setFragmentFunction(fragmentShader);
  assert(renderPipelineDescriptor);

  MTL::PixelFormat pixelFormat = metalLayer->pixelFormat();
  renderPipelineDescriptor->colorAttachments()->object(0)->setPixelFormat(
      pixelFormat);
  renderPipelineDescriptor->setSampleCount(sampleCount);
  renderPipelineDescriptor->setDepthAttachmentPixelFormat(
      MTL::PixelFormatDepth32Float);
  renderPipelineDescriptor->setTessellationOutputWindingOrder(
      MTL::WindingClockwise);

  NS::Error *error;
  metalRenderPS0 =
      metalDevice->newRenderPipelineState(renderPipelineDescriptor, &error);

  if (metalRenderPS0 == nil) {
    std::cout << "Error creating render pipeline state: " << error << std::endl;
    std::exit(0);
  }

  MTL::DepthStencilDescriptor *depthStencilDescriptor =
      MTL::DepthStencilDescriptor::alloc()->init();
  depthStencilDescriptor->setDepthCompareFunction(
      MTL::CompareFunctionLessEqual);
  depthStencilDescriptor->setDepthWriteEnabled(true);
  depthStencilState = metalDevice->newDepthStencilState(depthStencilDescriptor);

  renderPipelineDescriptor->release();
  depthStencilDescriptor->release();
  vertexShader->release();
  fragmentShader->release();
};

void MTLEngine::createLightSourceRenderPipeline() {
  MTL::Function *vertexShader = metalDefaultLibrary->newFunction(
      NS::String::string("lightVertexShader", NS::ASCIIStringEncoding));
  assert(vertexShader);
  MTL::Function *fragmentShader = metalDefaultLibrary->newFunction(
      NS::String::string("lightFragmentShader", NS::ASCIIStringEncoding));
  assert(fragmentShader);

  MTL::RenderPipelineDescriptor *renderPipelineDescriptor =
      MTL::RenderPipelineDescriptor::alloc()->init();
  renderPipelineDescriptor->setLabel(
      NS::String::string("Light Rendering Pipeline", NS::ASCIIStringEncoding));
  renderPipelineDescriptor->setVertexFunction(vertexShader);
  renderPipelineDescriptor->setFragmentFunction(fragmentShader);
  assert(renderPipelineDescriptor);

  MTL::PixelFormat pixelFormat = metalLayer->pixelFormat();
  renderPipelineDescriptor->colorAttachments()->object(0)->setPixelFormat(
      pixelFormat);
  renderPipelineDescriptor->setSampleCount(sampleCount);
  renderPipelineDescriptor->setDepthAttachmentPixelFormat(
      MTL::PixelFormatDepth32Float);
  renderPipelineDescriptor->setTessellationOutputWindingOrder(
      MTL::WindingClockwise);

  NS::Error *error;
  metalLightSourceRenderPSO =
      metalDevice->newRenderPipelineState(renderPipelineDescriptor, &error);

  if (metalLightSourceRenderPSO == nil) {
    std::cout << "Error creating light render pipeline state: " << error
              << std::endl;
    std::exit(0);
  }

  renderPipelineDescriptor->release();
  vertexShader->release();
  fragmentShader->release();
};

void MTLEngine::createDepthAndMSAATextures() {
  MTL::TextureDescriptor *msaaTextureDescriptor =
      MTL::TextureDescriptor::alloc()->init();
  CGSize drawableSize = metalLayer->drawableSize();

  msaaTextureDescriptor->setTextureType(MTL::TextureType2DMultisample);
  msaaTextureDescriptor->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
  msaaTextureDescriptor->setWidth(drawableSize.width);
  msaaTextureDescriptor->setHeight(drawableSize.height);
  msaaTextureDescriptor->setSampleCount(sampleCount);
  msaaTextureDescriptor->setUsage(MTL::TextureUsageRenderTarget);

  msaaRenderTargetTexture = metalDevice->newTexture(msaaTextureDescriptor);

  MTL::TextureDescriptor *depthTextureDescriptor =
      MTL::TextureDescriptor::alloc()->init();
  depthTextureDescriptor->setTextureType(MTL::TextureType2DMultisample);
  depthTextureDescriptor->setPixelFormat(MTL::PixelFormatDepth32Float);
  depthTextureDescriptor->setWidth(drawableSize.width);
  depthTextureDescriptor->setHeight(drawableSize.height);
  depthTextureDescriptor->setUsage(MTL::TextureUsageRenderTarget);
  depthTextureDescriptor->setSampleCount(sampleCount);

  depthTexture = metalDevice->newTexture(depthTextureDescriptor);

  msaaTextureDescriptor->release();
  depthTextureDescriptor->release();
}

void MTLEngine::createRenderPassDescriptor() {
  renderPassDescriptor = MTL::RenderPassDescriptor::alloc()->init();

  MTL::RenderPassColorAttachmentDescriptor *colorAttachment =
      renderPassDescriptor->colorAttachments()->object(0);
  MTL::RenderPassDepthAttachmentDescriptor *depthAttachment =
      renderPassDescriptor->depthAttachment();

  colorAttachment->setTexture(msaaRenderTargetTexture);
  colorAttachment->setResolveTexture(metalDrawable->texture());
  colorAttachment->setLoadAction(MTL::LoadActionClear);
  colorAttachment->setClearColor(
      MTL::ClearColor(41.0f / 255.0f, 42.0f / 255.0f, 48.0f / 255.0f, 1.0));
  colorAttachment->setStoreAction(MTL::StoreActionMultisampleResolve);

  depthAttachment->setTexture(depthTexture);
  depthAttachment->setLoadAction(MTL::LoadActionClear);
  depthAttachment->setStoreAction(MTL::StoreActionDontCare);
  depthAttachment->setClearDepth(1.0);
}

void MTLEngine::updateRenderPassDescriptor() {
  renderPassDescriptor->colorAttachments()->object(0)->setTexture(
      msaaRenderTargetTexture);
  renderPassDescriptor->colorAttachments()->object(0)->setResolveTexture(
      metalDrawable->texture());
  renderPassDescriptor->depthAttachment()->setTexture(depthTexture);
}

void MTLEngine::draw() { sendRenderCommand(); };

void MTLEngine::sendRenderCommand() {
  metalCommandBuffer = metalCommandQueue->commandBuffer();

  updateRenderPassDescriptor();

  MTL::RenderCommandEncoder *renderCommandEncoder =
      metalCommandBuffer->renderCommandEncoder(renderPassDescriptor);
  encodeRenderCommand(renderCommandEncoder);
  renderCommandEncoder->endEncoding();

  metalCommandBuffer->presentDrawable(metalDrawable);
  metalCommandBuffer->commit();
  metalCommandBuffer->waitUntilCompleted();
};

// Define the modal, view, perspective projection's here in the render command
void MTLEngine::encodeRenderCommand(
    MTL::RenderCommandEncoder *renderCommandEncoder) {

  // Moves the sphere 1 units down the negative z-axis
  matrix_float4x4 translationMatrix = matrix4x4_translation(0, 0, -1.5);
  matrix_float4x4 scaleMatrix = matrix4x4_scale(0.5, 0.5, 0.5);

  matrix_float4x4 sizeMatrix = matrix_multiply(translationMatrix, scaleMatrix);

  // Rotate the sphere by 90 degrees
  float angleInDegrees = glfwGetTime() / 2.0 * 45;
  float angleInRadians = angleInDegrees * M_PI / 180.0f;
  matrix_float4x4 rotationMatrix =
      matrix4x4_rotation(angleInRadians, 0.0, 1.0, 0.0);

  matrix_float4x4 modelMatrix = simd_mul(sizeMatrix, rotationMatrix);

  simd::float3 R = simd::float3{1, 0, 0};  // Unit-Right
  simd::float3 U = simd::float3{0, 1, 0};  // Unit-Up
  simd::float3 F = simd::float3{0, 0, -1}; // Unit-Forward
  simd::float3 P = simd::float3{0, 0, 0};  // Camera Position in World Space

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

  memcpy(sphereTransformationBuffer->contents(), &transformationData,
         sizeof(transformationData));

  // Sphere Vertex Shader Data
  simd_float4 lightColor = simd_make_float4(1.0, 1.0, 1.0, 1.0);
  simd_float4 lightPosition = simd_make_float4(-2.0, 0.5, -1.75, 1.0);
  simd_float4 cameraPosition = simd_make_float4(P.xyz, 1.0);

  renderCommandEncoder->setFragmentBytes(&lightColor, sizeof(lightColor), 0);
  renderCommandEncoder->setFragmentBytes(&lightPosition, sizeof(lightPosition),
                                         1);
  renderCommandEncoder->setFragmentBytes(&cameraPosition,
                                         sizeof(cameraPosition), 2);

  // Tell what winding mode we are using and instruct metal to cull faces we
  // can't see
  renderCommandEncoder->setFrontFacingWinding(MTL::WindingCounterClockwise);
  renderCommandEncoder->setCullMode(MTL::CullModeBack);
  // Uncomment to show the wireframe of the object we are rendering
  // renderCommandEncoder->setTriangleFillMode(MTL::TriangleFillModeLines);
  renderCommandEncoder->setRenderPipelineState(metalRenderPS0);
  renderCommandEncoder->setDepthStencilState(depthStencilState);
  renderCommandEncoder->setVertexBuffer(sphereVertexBuffer, 0, 0);
  renderCommandEncoder->setVertexBuffer(sphereTransformationBuffer, 0, 1);
  MTL::PrimitiveType typeTriangle = MTL::PrimitiveTypeTriangle;
  renderCommandEncoder->setFragmentTexture(marsTexture->texture, 0);
  renderCommandEncoder->drawPrimitives(typeTriangle, (NS::UInteger)0,
                                       vertexCount);

  // Light matrix
  scaleMatrix = matrix4x4_scale(0.25f, 0.25f, 0.25f);
  translationMatrix = matrix4x4_translation(lightPosition.xyz);
  modelMatrix = simd_mul(translationMatrix, scaleMatrix);

  renderCommandEncoder->setRenderPipelineState(metalLightSourceRenderPSO);
  transformationData = {modelMatrix, viewMatrix, perspectiveMatrix};
  memcpy(lightTransformationBuffer->contents(), &transformationData,
         sizeof(transformationData));
  renderCommandEncoder->setDepthStencilState(depthStencilState);
  renderCommandEncoder->setVertexBuffer(lightVertexBuffer, 0, 0);
  renderCommandEncoder->setVertexBuffer(lightTransformationBuffer, 0, 1);
  renderCommandEncoder->setFragmentBytes(&lightColor, sizeof(lightColor), 0);
  renderCommandEncoder->drawPrimitives(typeTriangle, (NS::UInteger)0,
                                       (NS::UInteger)36);
};
