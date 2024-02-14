#pragma once

#include <DirectXMath.h>

struct VertexShaderData
{
	DirectX::XMFLOAT4 colorTint;
	DirectX::XMMATRIX worldMatrix;
};