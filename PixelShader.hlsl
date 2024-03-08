#include "ShaderIncludes.hlsli"

#define MAX_SPECULAR_EXPONENT 256.0f

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
	Light lights[MAX_LIGHTS];
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
    input.normal = normalize(input.normal);
    
    float specularPower = (1.0f - roughness) * MAX_SPECULAR_EXPONENT;
    float3 viewVector = normalize(cameraPos - input.worldPosition);
    
	
    float3 color = ambientColor * colorTint.xyz;

    float3 lightDirection;
	for (int i = 0; i < numLights; i++)
	{
        if (0 == lights[i].type)
        {
            lightDirection = normalize(lights[i].direction);
            color += lights[i].intensity * 
            (saturate(Lambert(input.normal, lightDirection)) + Phong(input.normal, lightDirection, viewVector, specularPower)) * lights[i].color;
        }
        else if (1 == lights[i].type)
        {
            lightDirection = normalize(input.worldPosition - lights[i].position);
            color += lights[i].intensity * Attenuate(lights[i], input.worldPosition) * 
            (saturate(Lambert(input.normal, lightDirection)) + Phong(input.normal, lightDirection, viewVector, specularPower)) * lights[i].color;
        }
        else if (2 == lights[i].type)
        {
            //spot light {
            //get angle from center with max(dot(directionTolight, lightDirection), 0.0f);
            //then raise angle to power of falloff with pow(anglefromcenter, light.spotfalloff);
            //}
            /*
            float3 SpotLight(float3 lightDirection, float3 directionToLight)
            {
                float angleFromCenter = max(dot(directionToLight, lightDirection), 0.0f);
                return pow(angleFromCenter, light.spotfalloff);
            }
            */
        }
	}

    return float4(color, 1);
}