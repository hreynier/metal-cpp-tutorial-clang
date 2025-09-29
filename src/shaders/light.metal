#include <metal_stdlib>
using namespace metal;

#include "vertex_data.hpp"

struct LightVertexData {
    float4 position [[position]];
    float3 normal;
};

vertex LightVertexData lightVertexShader(uint vertexID [[vertex_id]],
                                        constant VertexData *vertexData [[buffer(0)]],
                                        constant TransformationData *transformationData [[buffer(1)]]
                                        ) {
    LightVertexData out;
    out.position = transformationData -> perspectiveMatrix * transformationData -> viewMatrix * transformationData -> modelMatrix * vertexData[vertexID].position;
    out.normal = vertexData[vertexID].normal.xyz;
    return out;
};

fragment float4 lightFragmentShader(LightVertexData in [[stage_in]],
                                    constant float4 &lightColor [[buffer(0)]]
                                    ) {
    return lightColor;
};
