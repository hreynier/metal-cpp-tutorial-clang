#pragma once
// Apple library containing math types that correspond directly with Metal
// shader datatypes, such as float4 vectors, float4x4 matrices, etc
#include <simd/simd.h>

// To render a texture, we need to pass the GPU some information about how we'd
// like to map the texture to our square. This is called a "uv" or "texture
// coordinates". For this, the four corners of our square will correspond
// directly with the four corners of the texture in uv coordinates (ranging from
// 0.0 to 1.0) (0,0) is the bottom left corner of the image (1,1) is the top
// right corner of the image
struct VertexData {
  // Define position vertices as a float4 vector
  simd::float4 position;
  // And texture coordinate as float2
  simd::float2 textureCoordinate;
  // Normal matrix
  simd::float4 normal;
};

//
struct TransformationData {
  simd::float4x4 modelMatrix;
  simd::float4x4 viewMatrix;
  simd::float4x4 perspectiveMatrix;
};
