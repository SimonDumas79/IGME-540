#pragma once

#include "DXCore.h"
#include <DirectXMath.h>
#include "Input.h"
#include "Transform.h"

class Camera
{
public:
	Camera(float x, float y, float z, float moveSpeed, float lookSpeed, float aspectRatio);
	~Camera();

	void Update(float dt);
	void UpdateViewMatrix();
	void UpdateProjectionMatrix();


private:
	DirectX::XMFLOAT4X4 viewMatrix;
	DirectX::XMFLOAT4X4 projMatrix;


	Transform transform;
};

