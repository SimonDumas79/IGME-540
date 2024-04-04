cbuffer DataPerEntity : register(b0)
{
    matrix viewMatrix;
    matrix projectionMatrix;
}

struct VertexShaderInput
{
    float3 position : POSITION; // XYZ position
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float3 tangent : TANGENT;
};

struct VertexToPixel
{
    float4 position : SV_POSITION;
    float3 sampleDir : DIRECTION;
};

VertexToPixel main(VertexShaderInput input)
{
    VertexToPixel output;
    
    matrix viewMatrixNoTranslation = viewMatrix;
    viewMatrixNoTranslation._14 = 0;
    viewMatrixNoTranslation._24 = 0;
    viewMatrixNoTranslation._34 = 0;
    
    output.position = mul(projectionMatrix, mul(viewMatrixNoTranslation, float4(input.position,1)));
    output.position.z = output.position.w;
    
    output.sampleDir = input.position;
    
    return output;
}