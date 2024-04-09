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

Texture2D SurfaceTexture : register(t0);
Texture2D SurfaceTextureSpecular : register(t1);
Texture2D SurfaceTextureNormal : register(t2);

SamplerState BasicSampler : register(s0);

// --------------------------------------------------------
// The entry point (main method) for our pixel shader
// 
// - Input is the data coming down the pipeline (defined by the struct)
// - Output is a single color (float4)
// - Has a special semantic (SV_TARGET), which means 
//    "put the output of this into the current render target"
// - Named "main" because that's the default the shader compiler looks for
// --------------------------------------------------------
float4 main(VertexToPixelWithNormalMap input) : SV_TARGET
{
    //get color from texture with texture.sample(basicsampler, uv)

    input.normal = normalize(input.normal);
    input.tangent = normalize(input.tangent);
    float3 unpackedNormal = SurfaceTextureNormal.Sample(BasicSampler, input.uv).xyz * 2 - 1;
    unpackedNormal = normalize(unpackedNormal);
    
    //normal
    float3 N = input.normal;
    //tangent
    float3 T = input.tangent;
    T = normalize(T - N * dot(T, N));
    //bitangent
    float3 B = cross(T, N);
    
    //rotation matrix for normal from normal map
    float3x3 TBN = float3x3(T, B, N);
    
    input.normal = mul(unpackedNormal, TBN);
    //get normal from map, normalize and unpack. 
    // T = normalize(input.tangent - N * dot(input.tangent, N))
    //get rotation matrix of the Normal, tangent, and bitangent using matrix of T, B, N
    //rotate the input.normal by multiplying by the tbn

    float specularPower = (1.0f - roughness) * MAX_SPECULAR_EXPONENT;
    float3 viewVector = normalize(cameraPos - input.worldPosition);

    float3 surfaceColor = pow(SurfaceTexture.Sample(BasicSampler, input.uv).xyz, 2.2f) * colorTint.xyz;
    float3 color = surfaceColor * ambientColor;

    float3 lightDirection;

    float specularFromMap = SurfaceTextureSpecular.Sample(BasicSampler, input.uv).x;
    for (int i = 0; i < numLights; i++)
    {
        if (0 == lights[i].type)
        {
            lightDirection = normalize(lights[i].direction);
            
            float diffuse = Lambert(input.normal, lightDirection);
            float specComponent = Phong(input.normal, lightDirection, viewVector, specularPower);
            
            color += lights[i].intensity *
            (saturate(diffuse) +
            (specComponent * any(diffuse)) * specularFromMap) * lights[i].color * surfaceColor;
        }
        else if (1 == lights[i].type)
        {
            lightDirection = normalize(input.worldPosition - lights[i].position);
            
            float diffuse = Lambert(input.normal, lightDirection);
            float specComponent = Phong(input.normal, lightDirection, viewVector, specularPower);
            
            color += lights[i].intensity * Attenuate(lights[i], input.worldPosition) *
            (saturate(diffuse) + (specComponent * any(diffuse))
            * specularFromMap) * lights[i].color * surfaceColor;
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

    color = pow(color, 1.0f / 2.2f);

    return float4(color, 1);
}