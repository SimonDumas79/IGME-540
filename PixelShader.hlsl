#include "ShaderIncludes.hlsli"
#define MAX_LIGHTS 128

// Struct representing the data we expect to receive from earlier pipeline stages
// - Should match the output of our corresponding vertex shader
// - The name of the struct itself is unimportant
// - The variable names don't have to match other shaders (just the semantics)
// - Each variable must have a semantic, which defines its usage

cbuffer DataFromCPU : register(b0)
{
	float4 colorTint;
	float3 cameraPos;
	float totalTime;
	Light lights[128];
	float3 ambientColor;
	float roughness;
	int numLights;
	float3 padding; //maintain 16 byte partitions
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
	float3 color = ambientColor * float3(colorTint.rgb);
	return float4(color, 1);
}