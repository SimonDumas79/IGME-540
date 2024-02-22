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
	void UpdateImGui(float deltaTime);
	void BuildUi();

	// Note the usage of ComPtr below
	//  - This is a smart pointer for objects that abide by the
	//     Component Object Model, which DirectX objects do
	//  - More info here: https://github.com/Microsoft/DirectXTK/wiki/ComPtr

	// Buffers to hold actual geometry data
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;

	//imgui section
	float bgColor[4] = { 0.4f, 0.6f, 0.75f, 1.0f }; // Cornflower Blue
	bool showWindow = true;
	int windowsToCreate = 0;
	char nextWindowTitle[256];
	char windowTitles[10][256];
	float offsetUI[3];
	float colorTintUI[4] = {1.0f, 1.0f, 1.0f, 1.0f};

	std::shared_ptr<Mesh>* meshes;
	std::shared_ptr<GameEntity>* entities;
	unsigned int entityCount;

	VertexShaderData vsData;

	std::shared_ptr<Camera> camera;

	std::shared_ptr<SimpleVertexShader> vs;
	std::shared_ptr<SimplePixelShader> ps;

};

