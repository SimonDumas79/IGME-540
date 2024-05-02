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
	DirectX::XMFLOAT4X4 worldInverseTranspose;

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
	DirectX::XMFLOAT3 GetUpVector();
	DirectX::XMFLOAT3 GetRightVector();
	DirectX::XMFLOAT4X4 GetWorldMatrix();
	DirectX::XMFLOAT4X4 GetWorldInverseTransposeMatrix();
	//setters
	void SetPosition(float x, float y, float z);
	void SetPosition(DirectX::XMFLOAT3 position);
	void SetRotation(float pitch, float yaw, float roll);
	void SetRotation(DirectX::XMFLOAT3 pitchYawRoll);
	void SetScale(float x, float y, float z);
	void SetScale(DirectX::XMFLOAT3 scale);

	//transformers
	
	//move in world space
	void MoveWorld(float x, float y, float z); 
	void MoveWorld(DirectX::XMFLOAT3 offset);

	//move in local space
	void MoveLocal(float x, float y, float z); 
	void MoveLocal(DirectX::XMFLOAT3 offset);

	void Rotate(float p, float y, float r);
	void Rotate(DirectX::XMFLOAT3 rotation);

	void Scale(float x, float y, float z);
	void Scale(DirectX::XMFLOAT3 scale);



};

