#pragma once

#include "DXCore.h"
#include "Mesh.h"
#include <DirectXMath.h>
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects
#include <memory>
#include "BufferStructs.h"
#include "Camera.h"
#include "SimpleShader.h"
#include "GameEntity.h"
#include "Material.h"

class Game 
	: public DXCore
{

public:
	Game(HINSTANCE hInstance);
	~Game();

	// Overridden setup and game loop methods, which
	// will be called automatically
	void Init();
	void OnResize();
	void Update(float deltaTime, float totalTime);
	void Draw(float deltaTime, float totalTime);

private:

	// Initialization helper methods - feel free to customize, combine, remove, etc.
	void LoadShaders(); 
	void CreateGeometry();
	void CreateMaterials();
	void UpdateImGui(float deltaTime);
	void BuildUi();

	// Note the usage of ComPtr below
	//  - This is a smart pointer for objects that abide by the
	//     Component Object Model, which DirectX objects do
	//  - More info here: https://github.com/Microsoft/DirectXTK/wiki/ComPtr

	//imgui section
	float bgColor[4] = { 0.4f, 0.6f, 0.75f, 1.0f }; // Cornflower Blue
	bool showWindow = true;
	int windowsToCreate = 0;
	char nextWindowTitle[256];
	char windowTitles[10][256];

	std::shared_ptr<Mesh>* meshes;
	std::shared_ptr<GameEntity>* entities;
	DirectX::XMFLOAT3* entityPositions;
	DirectX::XMFLOAT3* entityRotations;
	DirectX::XMFLOAT3* entityScales;
	unsigned int entityCount;
	unsigned int meshCount;

	std::vector<std::shared_ptr<Camera>> cameras;
	unsigned int activeCameraIndex;
	unsigned int numCameras;

	std::shared_ptr<SimpleVertexShader> vs;
	std::shared_ptr<SimplePixelShader> ps;

	std::vector<Material> materials;
};

