#include "ShaderIncludes.hlsli"
#define MAX_LIGHTS 128
#define LIGHT_TYPE_DIR   0
#define LIGHT_TYPE_POINT 1
#define LIGHT_TYPE_SPOT  2

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
	float3 ambientColor;
	float roughness;
	float3 padding; //maintain 16 byte partitions
}

cbuffer DataPerFrame : register(b1)
{
	Light lights[MAX_LIGHTS];
	Light directionalLight;
	int numLights;
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
	input.normal = normalize(input.normal);
	float3 color = Lambert(input.normal, normalize(lights[0].direction)) * lights[0].color * lights[0].intensity;

	for (int i = 0; i < numLights; i++)
	{
		switch (lights[i].type)
		{
			case LIGHT_TYPE_DIR:

				color = saturate(Lambert(input.normal, normalize(lights[i].direction))) * lights[i].color * lights[i].intensity;

				break;

			case LIGHT_TYPE_POINT:

				break;
			case LIGHT_TYPE_SPOT:

				break;
		}

	}

	return float4(color, 1);
}