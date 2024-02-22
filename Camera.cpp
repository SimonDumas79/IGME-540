#include "Camera.h"
using namespace DirectX;
Camera::Camera(
	float x, 
	float y, 
	float z, 
	float moveSpeed, 
	float lookSpeed, 
	float aspectRatio) : 
	movementSpeed(moveSpeed),
	mouseLookSpeed(lookSpeed)
{
	transform.SetPosition(x, y, z);
}

Camera::~Camera()
{
}

void Camera::Update(float dt)
{
	Input& input = Input::GetInstance();

	if (input.KeyDown('W')) { transform.MoveLocal(0, 0, dt * movementSpeed); }
	if (input.KeyDown('S')) { transform.MoveLocal(0, 0, dt * -movementSpeed); }
	if (input.KeyDown('A')) { transform.MoveLocal(dt * -movementSpeed, 0, 0); }
	if (input.KeyDown('D')) { transform.MoveLocal(dt * movementSpeed, 0, 0); }
	if (input.KeyDown(' ')) { transform.MoveWorld(0, dt * movementSpeed, 0); }
	if (input.KeyDown('X')) { transform.MoveWorld(0, dt * -movementSpeed, 0); }

	if (input.MouseLeftDown())
	{
		float xDelta = mouseLookSpeed * input.GetMouseXDelta();
		float yDelta = mouseLookSpeed * input.GetMouseYDelta();

		transform.Rotate(yDelta, xDelta, 0);

		XMFLOAT3 rot = transform.GetPitchYawRoll();
		if (rot.x > XM_PIDIV2) rot.x = XM_PIDIV2;
		else if (rot.x < -XM_PIDIV2) rot.x = -XM_PIDIV2;
		transform.SetRotation(rot);
	}


	UpdateViewMatrix();
}

void Camera::UpdateViewMatrix()
{
	//XMMatrixLookAtLH() - look at a point in space
	//XMMatrixLookToLH() - look toward a direction (vector)

	XMFLOAT3 pos = transform.GetPosition();
	XMFLOAT3 fwd = transform.GetForwardVector();

	XMMATRIX view = XMMatrixLookToLH(
		XMLoadFloat3(&pos),
		XMLoadFloat3(&fwd),
		XMVectorSet(0,1,0,0));
	
	XMStoreFloat4x4(&viewMatrix, view);

}

void Camera::UpdateProjectionMatrix(float aspectRatio)
{
	XMMATRIX proj = XMMatrixPerspectiveFovLH(
		XM_PIDIV4,		//fov in radians
		aspectRatio,	//aspect ratio
		0.01f,			//near clip, should never be 0
		1000.0f);		//far clip plane, dont go too high or distance precision will be lost

	XMStoreFloat4x4(&projMatrix, proj);
}

XMFLOAT4X4 Camera::GetView()
{
	return viewMatrix;
}

XMFLOAT4X4 Camera::GetProjection()
{
	return projMatrix;
}

Transform* Camera::GetTransform()
{
	return &transform;
}
