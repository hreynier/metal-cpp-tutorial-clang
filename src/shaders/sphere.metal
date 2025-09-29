#include <metal_stdlib>
using namespace metal;
#include <simd/simd.h>

#include "vertex_data.hpp"

// This is a new struct we are defining to hold the output of our data from the
// vertex shader.
// The position attribute is defined with two square brackets, indicating to
// Metal that we should apply perspective-division to it.
struct VertexOut {
  float4 position [[position]];
  // Since this member does not have a special attribute, the rasterizer
  // interpolates its value with the values of the other triangle vertices
  // and then passes the interpolated value to the fragment shader for each
  // fragment in the triangle.
  float2 textureCoordinate;
  float3 normal;
  float4 fragmentPosition; // Position in worldspace
};

vertex VertexOut sphereVertexShader(uint vertexID [[vertex_id]], constant VertexData *vertexData [[buffer(0)]], constant TransformationData *transformationData [[buffer(1)]]) {
    VertexOut out;
    float4 worldPosition = transformationData -> modelMatrix * vertexData[vertexID].position;
    out.position = transformationData -> perspectiveMatrix *
                  transformationData -> viewMatrix *
                  worldPosition;
    out.textureCoordinate = vertexData[vertexID].textureCoordinate;
    out.normal = (transformationData -> modelMatrix * float4(vertexData[vertexID].normal.xyz, 0.0)).xyz;
    out.fragmentPosition = worldPosition;
    return out;
};

fragment float4 sphereFragmentShader(VertexOut in [[stage_in]],
                                    texture2d<float> colorTexture [[texture(0)]],
                                    constant float4 &lightColor [[buffer(0)]],
                                    constant float4 &lightPosition [[buffer(1)]],
                                    constant float4 &cameraPosition [[buffer(2)]]
                                     ) {

    constexpr sampler textureSampler(mag_filter::linear, min_filter::linear);
    // Sample texture to obtain color
    const float4 colorSample =
        colorTexture.sample(textureSampler, in.textureCoordinate);

    // Ambient
    float ambientStrength = 0.2f;
    float4 ambient = ambientStrength * lightColor;

    // Diffuse
    float3 norm = normalize(in.normal.xyz);
    float4 lightDir = normalize(lightPosition - in.fragmentPosition);
    float diff = max(dot(norm, lightDir.xyz), 0.0);
    float4 diffuse = diff * lightColor;

    // Specular
    float specStrength = 1.0f;
    float4 viewDir = normalize(cameraPosition - in.fragmentPosition);
    float4 halfway = normalize(lightDir + viewDir);
    float specularIntensity = pow(max(dot(float4(norm, 1.0), halfway), 0.0), 32);
    float4 specular = specStrength * specularIntensity * lightColor;

    float4 finalCol = (ambient + diffuse + specular) * colorSample;
    return finalCol;
};
