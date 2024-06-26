#include "ShaderIncludes.hlsli"

#define MAX_LIGHTS 128

//change every entity
cbuffer DataPerEntity : register(b0)
{
    matrix worldMatrix;
    matrix worldInvTranspose;
}

//change once every draw call
cbuffer DataPerFrame : register(b1)
{
    matrix viewMatrix;
    matrix projectionMatrix;
    matrix lightViewMatrix;
    matrix lightProjMatrix;
}

// --------------------------------------------------------
// The entry point (main method) for our vertex shader
// 
// - Input is exactly one vertex worth of data (defined by a struct)
// - Output is a single struct of data to pass down the pipeline
// - Named "main" because that's the default the shader compiler looks for
// --------------------------------------------------------
VertexToPixelWithNormalMap main(VertexShaderInput input)
{
	// Set up output struct
    VertexToPixelWithNormalMap output;

	// Here we're essentially passing the input position directly through to the next
	// stage (rasterizer), though it needs to be a 4-component vector now.  
	// - To be considered within the bounds of the screen, the X and Y components 
	//   must be between -1 and 1.  
	// - The Z component must be between 0 and 1.  
	// - Each of these components is then automatically divided by the W component, 
	//   which we're leaving at 1.0 for now (this is more useful when dealing with 
	//   a perspective projection matrix, which we'll get to in the future).
	//output.screenPosition = float4(input.localPosition + offset, 1.0f);

    matrix wvp = mul(projectionMatrix, mul(viewMatrix, worldMatrix));
    output.screenPosition = mul(wvp, float4(input.localPosition, 1.0f));
    output.uv = input.uv;
    output.normal = mul((float3x3) worldInvTranspose, input.normal);
    output.tangent = mul((float3x3) worldMatrix, input.tangent);
	output.worldPosition = mul(worldMatrix, float4(input.localPosition, 1)).xyz;
	
    matrix shadowWVP = mul(lightProjMatrix, mul(lightViewMatrix, worldMatrix));
    output.shadowMapPos = mul(shadowWVP, float4(input.localPosition, 1.0f));
	
	// Whatever we return will make its way through the pipeline to the
	// next programmable stage we're using (the pixel shader for now)
    return output;
}