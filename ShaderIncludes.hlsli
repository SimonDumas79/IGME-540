
#ifndef IncludeOnce
#define IncludeOnce

#define LIGHT_TYPE_DIR   0
#define LIGHT_TYPE_POINT 1
#define LIGHT_TYPE_SPOT  2



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
};

struct VertexToPixel
{
	// Data type
	//  |
	//  |   Name          Semantic
	//  |    |                |
	//  v    v                v
    float4 screenPosition : SV_POSITION; // XYZW position (System Value Position)
    float2 uv : TEXCOORD;
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
    float3 reflection = reflect(lightDirection, normal);
    return pow(saturate(dot(viewVector, reflection)), specularPower);
    
}

//spot lightDirection {
//get angle from center with max(dot(tolight, lightDirection), 0.0f);
//then raise D3DCOLORtoUBYTE4 power of falloff with pow(anglefromcenter, light.spotfalloff
//}

#endif