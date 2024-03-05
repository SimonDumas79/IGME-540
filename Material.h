#pragma once
#include <wrl/client.h> 
#include <d3d11.h>
#include <DirectXMath.h>
#include "SimpleShader.h"
#include <memory>

class Material
{
public:

	Material(DirectX::XMFLOAT4 colorTint, 
		std::shared_ptr<SimpleVertexShader> vertexShader, 
		std::shared_ptr<SimplePixelShader> pixelShader,
		float roughness);

	~Material();



	void SetColorTint(DirectX::XMFLOAT4 colorTint);
	DirectX::XMFLOAT4 GetColorTint();

	void SetVertexShader(std::shared_ptr<SimpleVertexShader> vertexShader);
	std::shared_ptr<SimpleVertexShader> GetVertexShader();

	void SetPixelShader(std::shared_ptr<SimplePixelShader> pixelShader);
	std::shared_ptr<SimplePixelShader> GetPixelShader();

	void SetRoughness(float roughness);
	float GetRoughness();

private:

	DirectX::XMFLOAT4 colorTint;
	float roughness;

	std::shared_ptr<SimpleVertexShader> vs;
	std::shared_ptr<SimplePixelShader> ps;

};

