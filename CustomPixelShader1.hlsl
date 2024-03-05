#include "ShaderIncludes.hlsli"
// Struct representing the data we expect to receive from earlier pipeline stages
// - Should match the output of our corresponding vertex shader
// - The name of the struct itself is unimportant
// - The variable names don't have to match other shaders (just the semantics)
// - Each variable must have a semantic, which defines its usage

cbuffer DataFromCPU : register(b0)
{
    matrix worldMatrix;
    float4 colorTint;
    float totalTime;
}

float random(float2 s)
{
    return frac(sin(dot(s, float2(12.9898, 78.233))) * totalTime * 43758.5453123);
}
// --------------------------------------------------------
// The entry point (main method) for our pixel shader
// 
// - Input is the data coming down the pipeline (defined by the struct)
// - Output is a single color (float4)
// - Has a special semantic (SV_TARGET), which means 
//    "put the output of this into the current render target"
// - Named "main" because that's the default the shader compiler looks for
// --------------------------------------------------------
float4 main(VertexToPixel input) : SV_TARGET
{
    float4 color = sin(0.5f * totalTime) * colorTint;
    input.normal = abs(input.normal) / 2.0f;
    color += float4(input.normal, 1);
    
    if (abs(input.worldPosition.x - cos(totalTime)) < 1.7f && abs(input.worldPosition.z - sin(totalTime)) < 1.7f)
    {
        color *= float4(0.4, 0.4, 0.4, 0);
    }
    
    return color;
}