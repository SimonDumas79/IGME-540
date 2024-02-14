#include "GameEntity.h"

GameEntity::GameEntity(std::shared_ptr<Mesh> mesh)
{
	this->mesh = mesh;
}

GameEntity::~GameEntity()
{
	delete transform;
}

std::shared_ptr<Mesh> GameEntity::GetMesh()
{
	return std::shared_ptr<Mesh>();
}

Transform* GameEntity::GetTransform()
{
	return nullptr;
}

void GameEntity::Draw()
{
}
