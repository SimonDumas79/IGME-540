struct VertexToPixel
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

cbuffer externalData : register(b0)
{
    float levels;
}

Texture2D Pixels : register(t0);
SamplerState Sampler : register(s0);

float4 main(VertexToPixel input) : SV_TARGET
{
    //sample from the texture
    float4 fragColor = Pixels.Sample(Sampler, input.uv);

    //get the greyscale from the max value in fragcolor
    float greyscale = max(fragColor.r, max(fragColor.g, fragColor.b));

    //map the greyscale to its lower value and get the difference between greyscale and that value
    float lower = floor(greyscale * levels) / levels;
    float lowerDiff = abs(greyscale - lower);
    
    //map the greyscale to its upper value and get the difference between greyscale and that value
    float upper = ceil(greyscale * levels) / levels;
    float upperDiff = abs(upper - greyscale);
    
    //if the lower difference is greater than/equal to the upper difference use the lower val, otherwise use the upper val
    float level = lowerDiff <= upperDiff ? lower : upper;
    float adjustment = level / greyscale;
    
    //adjust the color
    fragColor.rgb *= adjustment;
    
    return fragColor;
}