#include "GameEntity.h"

GameEntity::GameEntity(std::shared_ptr<Mesh> mesh)
{
	this->mesh = mesh;
	transform = Transform();
}

GameEntity::~GameEntity()
{
	mesh->~Mesh();
	transform.~Transform();
}

std::shared_ptr<Mesh> GameEntity::GetMesh()
{
	return mesh;
}

Transform* GameEntity::GetTransform()
{
	return &transform;
}

void GameEntity::Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, std::shared_ptr<SimpleVertexShader> vs, std::shared_ptr<SimplePixelShader> ps, std::shared_ptr<Camera> camera, float totalTime)
{

	vs->SetShader();
	ps->SetShader();

	//provide data for vertex shader's cbuffer(s)

	vs->SetMatrix4x4("worldMatrix", transform.GetWorldMatrix());
	vs->SetMatrix4x4("viewMatrix", camera->GetView());
	vs->SetMatrix4x4("projectionMatrix", camera->GetProjection());
	//vs->SetFloat("totalTime", totalTime);

	//copy data to gpu
	vs->CopyAllBufferData();

	mesh->Draw(context);
	
}
