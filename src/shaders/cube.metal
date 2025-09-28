#include <metal_stdlib>
using namespace metal;
#include <simd/simd.h>

#include "vertex_data.hpp"

// This is a new struct we are defining to hold the output of our data from the
// vertex shader.
// The position attribute is defined with two square brackets, indicating to
// Metal that we should apply perspective-division to it.
struct VertexOut {
  // The [[position]] attribute of this member indicates that this value
  // is the clip space position of the vertex when this structure is returned
  // from the vertex function.
  float4 position [[position]];

  // Since this member does not have a special attribute, the rasterizer
  // interpolates its value with the values of the other triangle vertices
  // and then passes the interpolated value to the fragment shader for each
  // fragment in the triangle.
  float2 textureCoordinate;

  float3 normal;
  // Position in worldspace
  float4 fragmentPosition;
};

// Input to our new vertexShader is our vertexData, which is defined
// in the constant address space. This refers to buffers which are allocated in
// the read-only device memory.
vertex VertexOut cubeVertexShader(uint vertexID [[vertex_id]],
                                    constant VertexData *vertexData,
                                    constant TransformationData* transformationData) {
  VertexOut out;
  // Calculate worldspace position
  float4 worldPosition = transformationData -> modelMatrix * vertexData[vertexID].position;

  // Final clip position
  out.position = transformationData->perspectiveMatrix *
                 transformationData->viewMatrix *
                 worldPosition;

  out.textureCoordinate = vertexData[vertexID].textureCoordinate;

  // Transform normal to world space
  out.normal = (transformationData -> modelMatrix * float4(vertexData[vertexID].normal, 0.0)).xyz;

  // Pass world space position to fragment shader for lighting calcs
  out.fragmentPosition = worldPosition;
  return out;
};

// As input to the fragment shader, we are passing in our VertexOut data, as we
// want the interpolated texture-coordinate when sampling our texture.
//
// The [[stage_in]] attribute qualifier indicates that the `in` is an input
// from a previous pipeline stage.
//
// We also take in our texture as a `texture2d<float>` specifying with attribute
// [[texture(0)]] that we are taking the texture at index 0.
fragment float4 cubeFragmentShader(VertexOut in [[stage_in]],
                                     texture2d<float> colorTexture
                                     [[texture(0)]]) {

    // Camera at default position - slightly back and up to see the scene
    const float4 cameraPosition = float4(0.0, 0.0, 5.0, 1.0);

    // Warm Light
    const float4 lightColor = float4(1.0, 0.95, 0.8, 1.0);

    // Light positioned above and to the front-right for good illumination
    const float4 lightPosition = float4(2.0, 3.0, 4.0, 1.0);

    // We are setting the linear filtering for magnification and minifcation.
    // Mag = When a texture is displayed at a larger size than the original
    // texture Min = When a texture is displayed at a smaller size than the
    // original texture In each case, we are telling the shader to interpolate
    // between color values to create a smooth transition rather than a blocky
    // one.

    constexpr sampler textureSampler(mag_filter::linear, min_filter::linear);
    // Sample texture to obtain color
    const float4 colorSample =
        colorTexture.sample(textureSampler, in.textureCoordinate);

    // Ambient
    float ambientStrength = 0.4f;
    float4 ambient = ambientStrength * lightColor;

    // Diffuse
    float3 norm = simd::normalize(in.normal.xyz);
    float4 lightDir = simd::normalize(lightPosition - in.fragmentPosition);
    float diff = simd::max(simd::dot(norm, lightDir.xyz), 0.0);
    float4 diffuse = diff * lightColor;

    // Specular
    float specStrength = 1.0f;
    float4 viewDir = simd::normalize(cameraPosition - in.fragmentPosition);
    float4 reflectDir = simd::reflect(-lightDir, float4(norm, 1));
    float spec = simd::pow(simd::max(simd::dot(viewDir, reflectDir), 0.0), 16);
    float4 specular = specStrength * spec * lightColor;

    float4 finalCol = (ambient + diffuse + specular) * colorSample;
    return finalCol;
};
