#pragma once

#include "DXCore.h"
#include "Mesh.h"
#include <DirectXMath.h>
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects
#include <memory>
#include "Camera.h"
#include "SimpleShader.h"
#include "GameEntity.h"
#include "Material.h"
#include "Light.h"
#include "Sky.h"

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
	void CreateTextures();
	void UpdateImGui(float deltaTime);
	void BuildUi();
	void CreateLights();
	// Helper for creating a cubemap from 6 individual textures
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CreateCubemap(
		const wchar_t* right,
		const wchar_t* left,
		const wchar_t* up,
		const wchar_t* down,
		const wchar_t* front,
		const wchar_t* back);


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

	//shadow map shaders
	std::shared_ptr<SimpleVertexShader> shadowVS;


	std::shared_ptr<SimplePixelShader> PBRps;

	//textured pixel shader
	std::shared_ptr<SimplePixelShader> tps;

	// vertex and pixel shader with normal maps
	std::shared_ptr<SimpleVertexShader> nvs;
	std::shared_ptr<SimplePixelShader> nps; 

	//sky vertex and pixel shaders
	std::shared_ptr<SimpleVertexShader> skyVS;
	std::shared_ptr<SimplePixelShader> skyPS;

	std::vector<Material> materials;

	DirectX::XMFLOAT3 ambientColor;

	std::vector<Light> lights;
	DirectX::XMMATRIX lightViewMatrix;
	DirectX::XMMATRIX lightProjectionMatrix;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> shadowDSV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shadowSRV;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> shadowRasterizer;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> shadowSampler;
	int shadowMapResolution;
	

	//used for textures without a specular map
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> fullySpecularSRV;

	//used for textures without a normal map
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> flatNormalSRV;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> rustyMetalSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> rustyMetalSpecularSRV;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> brokenTilesSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> brokenTilesSpecularSRV;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> tilesSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> tilesSpecularSRV;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cushionSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cushionNormalSRV;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> rockSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> rockNormalSRV;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cobblestoneSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cobblestoneMetalSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cobblestoneNormalSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cobblestoneRoughnessSRV;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> bronzeSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> bronzeMetalSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> bronzeNormalSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> bronzeRoughnessSRV;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> floorSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> floorMetalSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> floorNormalSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> floorRoughnessSRV;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> paintSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> paintMetalSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> paintNormalSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> paintRoughnessSRV;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> roughSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> roughMetalSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> roughNormalSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> roughRoughnessSRV;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> scratchedSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> scratchedMetalSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> scratchedNormalSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> scratchedRoughnessSRV;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> woodSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> woodMetalSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> woodNormalSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> woodRoughnessSRV;

	Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState;

	//Skies
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cloudsBlueSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cloudsPinkSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> coldSunsetSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> planetSRV;


	std::shared_ptr<Sky> sky;

	//Post Processing 
	// Resources that are shared among all post processes
	Microsoft::WRL::ComPtr<ID3D11SamplerState> ppSampler;
	std::shared_ptr<SimpleVertexShader> ppVS;
	// Resources that are tied to a particular post process
	
	//Blur Post Processing resources
	std::shared_ptr<SimplePixelShader> ppBlurPS;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> ppBlurRTV; // For rendering
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ppBlurSRV; // For sampling

	//Pixelation Post Processing resources
	std::shared_ptr<SimplePixelShader> ppPixelatePS;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> ppPixelateRTV; // For rendering
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ppPixelateSRV; // For sampling

	//Posterization Post Processing resources
	std::shared_ptr<SimplePixelShader> ppPosterizePS;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> ppPosterizeRTV; // For rendering
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ppPosterizeSRV; // For sampling

	int blurRadius;
	int pixelSize;
	bool posterize;
	float posterizeLevel;

};

