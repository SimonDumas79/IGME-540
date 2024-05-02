struct VertexToPixel
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

cbuffer externalData : register(b0)
{
    int pixelSize;
    int textureWidth;
    int textureHeight;
}

Texture2D Pixels : register(t0);
SamplerState Sampler : register(s0);

float4 main(VertexToPixel input) : SV_TARGET
{
    //adjust the uv coords to be dependent on textureSize
    float2 adjustedCoordinates = float2(input.uv.x * textureWidth, input.uv.y * textureHeight);
    
    //get the nearest pizel position
    int x = int(adjustedCoordinates.x) % pixelSize;
    int y = int(adjustedCoordinates.y) % pixelSize;
    
    //get the offset
    x = pixelSize / 2 - x;
    y = pixelSize / 2 - y;
    
    //adjust the coordinates with the offsets
    adjustedCoordinates.x = adjustedCoordinates.x + x;
    adjustedCoordinates.y = adjustedCoordinates.y + y;
    
    //get the color from the adjusted coordinates
    float4 pixelColor = Pixels.Sample(Sampler, adjustedCoordinates / float2(textureWidth, textureHeight));
    
    return pixelColor;
}