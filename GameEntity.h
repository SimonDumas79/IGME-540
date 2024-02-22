#pragma once
#include <memory>
#include "Mesh.h"
#include "Transform.h"
#include "SimpleShader.h"
#include "Camera.h"


class GameEntity
{
private:

	Transform transform;
	std::shared_ptr<Mesh> mesh;

public:

	GameEntity(std::shared_ptr<Mesh> mesh);
	~GameEntity();

	std::shared_ptr<Mesh> GetMesh();
	Transform* GetTransform();

	void Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
		std::shared_ptr<SimpleVertexShader> vs,
		std::shared_ptr<SimplePixelShader> ps,
		std::shared_ptr<Camera> camera,
		float totalTime);

};

