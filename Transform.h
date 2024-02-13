#pragma once
#include <DirectXMath.h>

class Transform
{
private:

	//raw transform data
	DirectX::XMFLOAT3 translation;
	DirectX::XMFLOAT3 pitchYawRoll; //quaternion would be DirectX::XMFLOAT4 rotationQuaternion;
	DirectX::XMFLOAT3 scale;

	//combine into a single matrix
	DirectX::XMFLOAT4X4 worldMatrix;

	//does the matrix need to be changed
	bool matrixDirty;

	void UpdateWorldMatrix();



public:
	Transform();
	//no real need for a destructor

	//getters
	DirectX::XMFLOAT3 GetPosition();
	DirectX::XMFLOAT3 GetPitchYawRoll();
	DirectX::XMFLOAT3 GetScale();
	DirectX::XMFLOAT3 GetForwardVector();
	DirectX::XMFLOAT4X4 GetWorldMatrix();

	//setters
	void SetPosition(float x, float y, float z);
	void SetRotation(float pitch, float yaw, float roll);
	void SetScale(float x, float y, float z);

	//transformers
	void MoveWorld(float x, float y, float z); //move in world rotation
	void MoveLocal(float x, float y, float z); //move in local rotation
	void Rotate(float p, float y, float r);
	void Scale(float x, float y, float z);



};

