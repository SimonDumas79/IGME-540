#include "Transform.h"
using namespace DirectX;

Transform::Transform() :
	translation(0, 0, 0),
	pitchYawRoll(0, 0, 0),
	scale(1, 1, 1),
	matrixDirty(false)
{
	XMStoreFloat4x4(&worldMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&worldInverseTranspose, XMMatrixIdentity());
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

DirectX::XMFLOAT3 Transform::GetForwardVector()
{
	//local forward vector
	XMVECTOR fwd = XMVectorSet(0, 0, 1, 0);

	//quaternion for current rotation in world space
	XMVECTOR rotQuat = XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&pitchYawRoll));

	//rotate default forward by the actual rotation
	XMFLOAT3 forwardVector;
	XMStoreFloat3(&forwardVector, XMVector3Rotate(fwd, rotQuat));
	return forwardVector;
}

DirectX::XMFLOAT3 Transform::GetUpVector()
{
	return DirectX::XMFLOAT3();
}

DirectX::XMFLOAT3 Transform::GetRightVector()
{
	return DirectX::XMFLOAT3();
}

DirectX::XMFLOAT4X4 Transform::GetWorldMatrix()
{
	UpdateWorldMatrix();

	return worldMatrix;
}

DirectX::XMFLOAT4X4 Transform::GetWorldInverseTransposeMatrix()
{
	return worldInverseTranspose;
}

void Transform::SetPosition(float x, float y, float z)
{
	translation.x = x;
	translation.y = y;
	translation.z = z;
	matrixDirty = true;
}

void Transform::SetPosition(DirectX::XMFLOAT3 position)
{
	translation = position;
	matrixDirty = true;
}

void Transform::SetRotation(float pitch, float yaw, float roll)
{
	pitchYawRoll.x = pitch;
	pitchYawRoll.y = yaw;
	pitchYawRoll.z = roll;
	matrixDirty = true;
}

void Transform::SetRotation(DirectX::XMFLOAT3 pitchYawRoll)
{
	this->pitchYawRoll = pitchYawRoll;
	matrixDirty = true;
}

void Transform::SetScale(float x, float y, float z)
{
	scale.x = x;
	scale.y = y;
	scale.z = z;
	matrixDirty = true;
}

void Transform::SetScale(DirectX::XMFLOAT3 scale)
{
	this->scale = scale;
	matrixDirty = true;
}

void Transform::MoveWorld(float x, float y, float z)
{
	translation.x += x;
	translation.y += y;
	translation.z += z;
	matrixDirty = true;
}

void Transform::MoveWorld(DirectX::XMFLOAT3 offset)
{

}

void Transform::MoveLocal(float x, float y, float z)
{
	XMVECTOR movement = XMVectorSet(x, y, z, 0);
	XMVECTOR rotQuat = XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&pitchYawRoll));

	//rotate the movement to make it relative
	XMVECTOR dir = XMVector3Rotate(movement, rotQuat);

	//add the direction to our position
	XMStoreFloat3(&translation, XMLoadFloat3(&translation) + dir);

	matrixDirty = true;
}

void Transform::MoveLocal(DirectX::XMFLOAT3 offset)
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

void Transform::Rotate(DirectX::XMFLOAT3 rotation)
{
	pitchYawRoll.x += rotation.x;
	pitchYawRoll.y += rotation.y;
	pitchYawRoll.z += rotation.z;
	matrixDirty = true;
}

void Transform::Scale(float x, float y, float z)
{
	scale.x *= x;
	scale.y *= y;
	scale.z *= z;
	matrixDirty = true;
}

void Transform::Scale(DirectX::XMFLOAT3 scale)
{
	scale.x *= scale.x;
	scale.y *= scale.y;
	scale.z *= scale.z;
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
	XMStoreFloat4x4(&worldInverseTranspose,
		XMMatrixInverse(0, XMMatrixTranspose(worldMat)));
}