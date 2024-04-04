
#ifndef IncludeOnce
#define IncludeOnce

struct VertexShaderInput
{
	// Data type
	//  |
	//  |   Name          Semantic
	//  |    |                |
	//  v    v                v
    float3 localPosition : POSITION; // XYZ position
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float3 tangent : TANGENT;
};

struct VertexToPixel
{
	// Data type
	//  |
	//  |   Name          Semantic
	//  |    |                |
	//  v    v                v
    float4 screenPosition : SV_POSITION; // XYZW position (System Value Position)
    float3 worldPosition : POSITION; // XYZ position
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

struct VertexToPixelWithNormalMap
{
	// Data type
	//  |
	//  |   Name          Semantic
	//  |    |                |
	//  v    v                v
    float4 screenPosition : SV_POSITION; // XYZW position (System Value Position)
    float3 worldPosition : POSITION; // XYZ position
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float3 tangent : TANGENT;
};

struct Light
{
    int type;
    float3 direction;
    float range;
    float3 position;
    float intensity;
    float3 color;
	float SpotFalloff;
    float3 padding;
};

float Lambert(float3 normal, float3 lightDirection)
{
    //get the opposite direction of the light to get the direction to the light
    return dot(normal, -lightDirection);
}

float Phong(float3 normal, float3 lightDirection, float3 viewVector, float specularPower)
{
    if(specularPower >= .05)
    {
        float3 reflection = reflect(lightDirection, normal);
        return pow(saturate(dot(reflection, viewVector)), specularPower);
    }
    return 0;
}

float Attenuate(Light light, float3 worldPos)
{
    float dist = distance(light.position, worldPos);
    float att = saturate(1.0f - (dist * dist / (light.range * light.range)));
    return att * att;
}

#endif