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
    Light lights[MAX_LIGHTS];
    int numLights;
    float3 padding;
}

Texture2D Albedo : register(t0);
Texture2D NormalMap : register(t1);
Texture2D RoughnessMap : register(t2);
Texture2D MetalnessMap : register(t3);
Texture2D ShadowMap : register(t4);

SamplerState BasicSampler : register(s0);
SamplerComparisonState ShadowSampler : register(s1);

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
   
    // Perform the perspective divide (divide by W) ourselves
    input.shadowMapPos /= input.shadowMapPos.w;
    // Convert the normalized device coordinates to UVs for sampling
    float2 shadowUV = input.shadowMapPos.xy * 0.5f + 0.5f;
    shadowUV.y = 1 - shadowUV.y; // Flip the Y
    // Grab the distances we need: light-to-pixel and closest-surface
    float distToLight = input.shadowMapPos.z;
    // Get a ratio of comparison results using SampleCmpLevelZero()
    float shadowAmount = ShadowMap.SampleCmpLevelZero(
        ShadowSampler,
        shadowUV,
        distToLight).r;
    
    
    
    float3 unpackedNormal = NormalMap.Sample(BasicSampler, input.uv).xyz * 2 - 1;
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
    
    float roughness = RoughnessMap.Sample(BasicSampler, input.uv).x;
    float metalness = MetalnessMap.Sample(BasicSampler, input.uv).x;
    float3 surfaceColor = pow(Albedo.Sample(BasicSampler, input.uv).xyz, 2.2f) * colorTint.xyz;
    float3 specularColor = lerp(F0_NON_METAL, surfaceColor, metalness);
    
    
    float3 viewVector = normalize(cameraPos - input.worldPosition);

    float3 color;

    float3 lightDirection;
    for (int i = 0; i < numLights; i++)
    {
        if (0 == lights[i].type)
        {
            lightDirection = normalize(lights[i].direction);

            float diffuse = DiffusePBR(input.normal, -lightDirection);
            float3 F;
            float3 specComponent = MicrofacetBRDF(input.normal, -lightDirection, viewVector, roughness, specularColor, F);
            
            // Calculate diffuse with energy conservation, including cutting diffuse for metals
            float3 balancedDiffuse = DiffuseEnergyConserve(diffuse, F, metalness);
            
            // correct for normals from normal maps picking up spec on opposite side
            specComponent *= any(diffuse);
            
            float3 total = (balancedDiffuse * surfaceColor + specComponent) * lights[i].intensity * lights[i].color;
            
            if (i == 0)
            {
                total *= shadowAmount;
            }
            
            color += total;
        }
        else if (1 == lights[i].type)
        {
            lightDirection = normalize(input.worldPosition - lights[i].position);

            float diffuse = DiffusePBR(input.normal, -lightDirection);
            float3 F;
            float3 specComponent = MicrofacetBRDF(input.normal, -lightDirection, viewVector, roughness, specularColor, F);
            
            // Calculate diffuse with energy conservation, including cutting diffuse for metals
            float3 balancedDiffuse = DiffuseEnergyConserve(diffuse, F, metalness);
            
            // correct for normals from normal maps picking up spec on opposite side
            specComponent *= any(diffuse);
            
            float3 total = (balancedDiffuse * surfaceColor + specComponent) * lights[i].intensity * lights[i].color * Attenuate(lights[i], input.worldPosition);
            
            color += total;
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