#include "Transform.h"
using namespace DirectX;

Transform::Transform() :
	translation(0, 0, 0),
	pitchYawRoll(0, 0, 0),
	scale(1, 1, 1),
	matrixDirty(false);
{
	XMStoreFloat4x4(&worldMatrix, XMMatrixIdentity());
}

DirectX::XMFLOAT3 Transform::GetPosition()
{
	return translation;
}

DirectX::XMFLOAT3 Transform::GetPitchYawRoll()
{
	return pitchYawRoll;
}

DirectX::XMFLOAT3 Transform::GetScale()
{
	return scale;
}

DirectX::XMFLOAT4X4 Transform::GetWorldMatrix()
{
	UpdateWorldMatrix();

	return worldMatrix;
}

void Transform::SetPosition(float x, float y, float z)
{
	translation.x = x;
	translation.y = y;
	translation.z = z;
	matrixDirty = true;
}

void Transform::SetRotation(float pitch, float yaw, float roll)
{
	pitchYawRoll.x = pitch;
	pitchYawRoll.y = yaw;
	pitchYawRoll.z = roll;
	matrixDirty = true;
}

void Transform::SetScale(float x, float y, float z)
{
	scale.x = x;
	scale.y = y;
	scale.z = z;
	matrixDirty = true;
}

void Transform::MoveWorld(float x, float y, float z)
{
	translation.x += x;
	translation.y += y;
	translation.z += z;
	matrixDirty = true;
}

void Transform::MoveLocal(float x, float y, float z)
{

	matrixDirty = true;
}

void Transform::Rotate(float p, float y, float r)
{
	pitchYawRoll.x += p;
	pitchYawRoll.y += y;
	pitchYawRoll.z += r;
	matrixDirty = true;
}

void Transform::Scale(float x, float y, float z)
{
	scale.x *= x;
	scale.y *= y;
	scale.z *= z;
	matrixDirty = true;
}

void Transform::UpdateWorldMatrix()
{
	if (!matrixDirty)
	{
		return;
	}

	//create world matrix

	XMMATRIX t = XMMatrixTranslationFromVector(XMLoadFloat3(&translation));
	XMMATRIX s = XMMatrixScalingFromVector(XMLoadFloat3(&scale));
	XMMATRIX r = XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&pitchYawRoll));

	XMMATRIX worldMat = (s * r) * t;

	XMStoreFloat4x4(&worldMatrix, worldMat);
}