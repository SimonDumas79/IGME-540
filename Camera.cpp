#include "Camera.h"

Camera::Camera(float x, float y, float z, float moveSpeed, float lookSpeed, float aspectRatio)
{
	transform.SetPosition(x, y, z);
}

Camera::~Camera()
{
}

void Camera::Update(float dt)
{
	Input& input = Input::GetInstance();

	UpdateViewMatrix();
}

void Camera::UpdateViewMatrix()
{
	//XMMatrixLookAtLH() - look at a point in space
	//XMMatrixLookToLH() - look toward a direction (vector)

	DirectX::XMFLOAT3 pos = transform.GetPosition();
	DirectX::XMFLOAT3 fwd = transform.GetForwardVector();

	DirectX::XMMATRIX view = DirectX::XMMatrixLookToLH(
		DirectX::XMLoadFloat3(&pos),
		DirectX::XMLoadFloat3(&fwd),
		DirectX::XMVectorSet(0,1,0,0));
	
	DirectX::XMStoreFloat4x4(&viewMatrix, view);

}

void Camera::UpdateProjectionMatrix()
{

}
