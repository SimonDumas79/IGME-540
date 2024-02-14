#pragma once
#include <memory>
#include "Mesh.h"
#include "Transform.h"

class GameEntity
{
private:

	Transform* transform;
	std::shared_ptr<Mesh> mesh;

public:

	GameEntity(std::shared_ptr<Mesh> mesh);
	~GameEntity();

	std::shared_ptr<Mesh> GetMesh();
	Transform* GetTransform();

	void Draw();

};

