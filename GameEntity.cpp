#include "GameEntity.h"

GameEntity::GameEntity(std::shared_ptr<Mesh> mesh,
	std::shared_ptr<Material> material):
	mesh(mesh),
	material(material)
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

std::shared_ptr<Material> GameEntity::GetMaterial()
{
	return material;
}

void GameEntity::SetMaterial(std::shared_ptr<Material> material)
{
	this->material.reset();
	this->material = material;
}

void GameEntity::Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, std::shared_ptr<Camera> camera, float totalTime)
{

	material->GetVertexShader()->SetShader();
	material->GetPixelShader()->SetShader();

	//provide data for vertex shader's cbuffer(s)

	material->GetVertexShader()->SetMatrix4x4("worldMatrix", transform.GetWorldMatrix());
	material->GetVertexShader()->SetMatrix4x4("worldInvTranspose", transform.GetWorldInverseTransposeMatrix());
	//vs->SetFloat("totalTime", totalTime);

	//copy data to gpu
	material->GetVertexShader()->CopyAllBufferData();

	material->GetPixelShader()->SetFloat4("colorTint", material->GetColorTint());
	material->GetPixelShader()->SetFloat("totalTime", totalTime);
	material->GetPixelShader()->SetMatrix4x4("worldMatrix", transform.GetWorldMatrix());
	material->GetPixelShader()->SetFloat3("cameraPos", camera->GetTransform()->GetPosition());
	material->GetPixelShader()->SetFloat("roughness", material->GetRoughness());

	//copy data to gpu
	material->GetPixelShader()->CopyAllBufferData();

	mesh->Draw(context);
	
}
