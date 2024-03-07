
#ifndef IncludeOnce
#define IncludeOnce

#define MAX_SPECULAR_EXPONENT 256.0f


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
    float3 worldPosition : POSITION; // XYZ position
    float3 normal : NORMAL;
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
float3 PointLight()
{
    
}

float3 DirectionalLight()
{
    
}


#endif