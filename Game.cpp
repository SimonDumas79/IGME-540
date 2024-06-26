#include "Game.h"
#include "Vertex.h"
#include "Input.h"
#include "PathHelpers.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"
#include "Transform.h"
#include <iostream>
#include "WICTextureLoader.h"

// Needed for a helper function to load pre-compiled shader files
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>

// For the DirectX Math library
using namespace DirectX;

// --------------------------------------------------------
// Constructor
//
// DXCore (base class) constructor will set up underlying fields.
// Direct3D itself, and our window, are not ready at this point!
//
// hInstance - the application's OS-level handle (unique ID)
// --------------------------------------------------------
Game::Game(HINSTANCE hInstance)
	: DXCore(
		hInstance,			// The application's handle
		L"DirectX Game",	// Text for the window's title bar (as a wide-character string)
		1280,				// Width of the window's client area
		720,				// Height of the window's client area
		false,				// Sync the framerate to the monitor refresh? (lock framerate)
		true)				// Show extra stats (fps) in title bar?
{
#if defined(DEBUG) || defined(_DEBUG)
	// Do we want a console window?  Probably only in debug mode

	CreateConsoleWindow(500, 120, 32, 120);
	printf("Console window created successfully.  Feel free to printf() here.\n");


#endif
	//Giving all variables initial values
	ambientColor = DirectX::XMFLOAT3(0.05f, 0.05f, 0.05f);
	entityCount = 0;
	activeCameraIndex = 0;
	meshes = new std::shared_ptr<Mesh>[entityCount];
	entities = new std::shared_ptr<GameEntity>[entityCount];
	std::memset(nextWindowTitle, '\0', sizeof(nextWindowTitle));
	std::memset(windowTitles[0], '\0', sizeof(windowTitles[0]));
	for (int i = 0; i < 10; ++i) {
		std::memset(windowTitles[i], '\0', sizeof(windowTitles[i]));
	}
}

// --------------------------------------------------------
// Destructor - Clean up anything our game has created:
//  - Delete all objects manually created within this class
//  - Release() all Direct3D objects created within this class
// --------------------------------------------------------
Game::~Game()
{
	// Call delete or delete[] on any objects or arrays you've
	// created using new or new[] within this class
	// - Note: this is unnecessary if using smart pointers

	// Call Release() on any Direct3D objects made within this class
	// - Note: this is unnecessary for D3D objects stored in ComPtrs

	// ImGui clean up
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	delete[] meshes;
	meshes = nullptr;

	delete[] entities;
	entities = nullptr;

	delete[] entityPositions;
	delete[] entityRotations;
	delete[] entityScales;

}

// --------------------------------------------------------
// Called once per program, after Direct3D and the window
// are initialized but before the game loop.
// --------------------------------------------------------
void Game::Init()
{

	cameras.push_back(std::make_shared<Camera>(
		0.0f, 0.0f, -10.5f,
		5.0f,
		0.002f,
		(float)windowWidth / windowHeight));
	cameras.push_back(std::make_shared<Camera>(
		-5.0f, 0.0f, -5.5f,
		5.0f,
		0.002f,
		(float)windowWidth / windowHeight));
	cameras.push_back(std::make_shared<Camera>(
		5.0f, 0.0f, -5.5f,
		5.0f,
		0.002f,
		(float)windowWidth / windowHeight));
	cameras[1]->GetTransform()->SetRotation(DirectX::XMFLOAT3(0, XM_PIDIV4, 0));
	cameras[2]->GetTransform()->SetRotation(DirectX::XMFLOAT3(0, -XM_PIDIV4, 0));

	numCameras = 3;

	//load shaders into pointers
	LoadShaders();

	CreateTextures();

	CreateMaterials();

	CreateLights();

	//create game entities
	CreateGeometry();

	//shadow rasterizer
	D3D11_RASTERIZER_DESC shadowRastDesc = {};
	shadowRastDesc.FillMode = D3D11_FILL_SOLID;
	shadowRastDesc.CullMode = D3D11_CULL_BACK;
	shadowRastDesc.DepthClipEnable = true;
	shadowRastDesc.DepthBias = 1000; // Min. precision units, not world units!
	shadowRastDesc.SlopeScaledDepthBias = 1.0f; // Bias more based on slope
	device->CreateRasterizerState(&shadowRastDesc, &shadowRasterizer);

	//shadow sampler
	D3D11_SAMPLER_DESC shadowSampDesc = {};
	shadowSampDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	shadowSampDesc.ComparisonFunc = D3D11_COMPARISON_LESS;
	shadowSampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSampDesc.BorderColor[0] = 1.0f; // Only need the first component
	device->CreateSamplerState(&shadowSampDesc, &shadowSampler);

	//post processing sampler
	D3D11_SAMPLER_DESC ppSampDesc = {};
	ppSampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	ppSampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	ppSampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	ppSampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	ppSampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	device->CreateSamplerState(&ppSampDesc, ppSampler.GetAddressOf());

	//No blur to start
	blurRadius = 0;

	//Pixel Size is 1 to start
	pixelSize = 1;

	//posterize disabled at start
	posterize = false;

	//posterization level set to 10 by default
	posterizeLevel = 10;


	// Initialize ImGui itself & platform/renderer backends
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX11_Init(device.Get(), context.Get());


	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();
	//ImGui::StyleColorsClassic();

	//Fixes "Current Primitive Technology is not valid" error
	context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

// --------------------------------------------------------
// Loads shaders from compiled shader object (.cso) files
// and also created the Input Layout that describes our 
// vertex data to the rendering pipeline. 
// - Input Layout creation is done here because it must 
//    be verified against vertex shader byte code
// - We'll have that byte code already loaded below
// --------------------------------------------------------
void Game::LoadShaders()
{
	ps = std::make_shared<SimplePixelShader>(
		device, context, FixPath(L"PixelShader.cso").c_str());
	tps = std::make_shared<SimplePixelShader>(
		device, context, FixPath(L"TexturedPixelShader.cso").c_str());
	nps = std::make_shared<SimplePixelShader>(
		device, context, FixPath(L"NormalMapPixelShader.cso").c_str());

	skyPS = std::make_shared<SimplePixelShader>(
		device, context, FixPath(L"SkyPixelShader.cso").c_str());
	//customPixelShader1 = std::make_shared<SimplePixelShader>(device, context, FixPath(L"CustomPixelShader1.cso").c_str());

	vs = std::make_shared<SimpleVertexShader>(
		device, context, FixPath(L"VertexShader.cso").c_str());

	nvs = std::make_shared<SimpleVertexShader>(
		device, context, FixPath(L"NormalMapVertexShader.cso").c_str());

	skyVS = std::make_shared<SimpleVertexShader>(
		device, context, FixPath(L"SkyVertexShader.cso").c_str());

	shadowVS = std::make_shared<SimpleVertexShader>(
		device, context, FixPath(L"ShadowMapVertexShader.cso").c_str());

	PBRps = std::make_shared<SimplePixelShader>(
		device, context, FixPath(L"PBRPixelShader.cso").c_str());

	ppVS = std::make_shared<SimpleVertexShader>(
		device, context, FixPath(L"PostProcessingVertexShader.cso").c_str());

	ppBlurPS = std::make_shared<SimplePixelShader>(
		device, context, FixPath(L"BlurPixelShader.cso").c_str());

	ppPixelatePS = std::make_shared<SimplePixelShader>(
		device, context, FixPath(L"PixelatePixelShader.cso").c_str());

	ppPosterizePS = std::make_shared<SimplePixelShader>(
		device, context, FixPath(L"PosterizationPixelShader.cso").c_str());

	/*
	* Creates Shaders manually
	// BLOBs (or Binary Large OBjects) for reading raw data from external files
	// - This is a simplified way of handling big chunks of external data
	// - Literally just a big array of bytes read from a file
	ID3DBlob* pixelShaderBlob;
	ID3DBlob* vertexShaderBlob;

	// Loading shaders
	//  - Visual Studio will compile our shaders at build time
	//  - They are saved as .cso (Compiled Shader Object) files
	//  - We need to load them when the application starts
	{
		// Read our compiled shader code files into blobs
		// - Essentially just "open the file and plop its contents here"
		// - Uses the custom FixPath() helper from Helpers.h to ensure relative paths
		// - Note the "L" before the string - this tells the compiler the string uses wide characters
		D3DReadFileToBlob(FixPath(L"PixelShader.cso").c_str(), &pixelShaderBlob);
		D3DReadFileToBlob(FixPath(L"VertexShader.cso").c_str(), &vertexShaderBlob);

		// Create the actual Direct3D shaders on the GPU
		device->CreatePixelShader(
			pixelShaderBlob->GetBufferPointer(),	// Pointer to blob's contents
			pixelShaderBlob->GetBufferSize(),		// How big is that data?
			0,										// No classes in this shader
			pixelShader.GetAddressOf());			// Address of the ID3D11PixelShader pointer

		device->CreateVertexShader(
			vertexShaderBlob->GetBufferPointer(),	// Get a pointer to the blob's contents
			vertexShaderBlob->GetBufferSize(),		// How big is that data?
			0,										// No classes in this shader
			vertexShader.GetAddressOf());			// The address of the ID3D11VertexShader pointer
	}


	// Create an input layout
	//  - This describes the layout of data sent to a vertex shader
	//  - In other words, it describes how to interpret data (numbers) in a vertex buffer
	//  - Doing this NOW because it requires a vertex shader's byte code to verify against!
	//  - Luckily, we already have that loaded (the vertex shader blob above)
	{
		D3D11_INPUT_ELEMENT_DESC inputElements[2] = {};

		// Set up the first element - a position, which is 3 float values
		inputElements[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;				// Most formats are described as color channels; really it just means "Three 32-bit floats"
		inputElements[0].SemanticName = "POSITION";							// This is "POSITION" - needs to match the semantics in our vertex shader input!
		inputElements[0].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;	// How far into the vertex is this?  Assume it's after the previous element

		// Set up the second element - a color, which is 4 more float values
		inputElements[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;			// 4x 32-bit floats
		inputElements[1].SemanticName = "COLOR";							// Match our vertex shader input!
		inputElements[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;	// After the previous element

		// Create the input layout, verifying our description against actual shader code
		device->CreateInputLayout(
			inputElements,							// An array of descriptions
			2,										// How many elements in that array?
			vertexShaderBlob->GetBufferPointer(),	// Pointer to the code of a shader that uses this layout
			vertexShaderBlob->GetBufferSize(),		// Size of the shader code that uses this layout
			inputLayout.GetAddressOf());			// Address of the resulting ID3D11InputLayout pointer
	}
	*/
}


void Game::CreateTextures()
{
	shadowMapResolution = 1024;
	//create shadow map texture
	D3D11_TEXTURE2D_DESC shadowMapDesc = {};
	shadowMapDesc.Width = shadowMapResolution;
	shadowMapDesc.Height = shadowMapResolution;
	shadowMapDesc.ArraySize = 1;
	shadowMapDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	shadowMapDesc.CPUAccessFlags = 0;
	shadowMapDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	shadowMapDesc.MipLevels = 1;
	shadowMapDesc.MiscFlags = 0;
	shadowMapDesc.SampleDesc.Count = 1;
	shadowMapDesc.SampleDesc.Quality = 0;
	shadowMapDesc.Usage = D3D11_USAGE_DEFAULT;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> shadowTexture;
	device->CreateTexture2D(&shadowMapDesc, 0, shadowTexture.GetAddressOf());

	//Blur Post Processing
	{
		//create post processing texture
		D3D11_TEXTURE2D_DESC ppBlurTextureDesc = {};
		ppBlurTextureDesc.Width = windowWidth;
		ppBlurTextureDesc.Height = windowHeight;
		ppBlurTextureDesc.ArraySize = 1;
		ppBlurTextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		ppBlurTextureDesc.CPUAccessFlags = 0;
		ppBlurTextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		ppBlurTextureDesc.MipLevels = 1;
		ppBlurTextureDesc.MiscFlags = 0;
		ppBlurTextureDesc.SampleDesc.Count = 1;
		ppBlurTextureDesc.SampleDesc.Quality = 0;
		ppBlurTextureDesc.Usage = D3D11_USAGE_DEFAULT;
		// Create the resource (no need to track it after the views are created below)
		Microsoft::WRL::ComPtr<ID3D11Texture2D> ppBlurTexture;
		device->CreateTexture2D(&ppBlurTextureDesc, 0, ppBlurTexture.GetAddressOf());

		// Create the Render Target View
		D3D11_RENDER_TARGET_VIEW_DESC rtvBlurDesc = {};
		rtvBlurDesc.Format = ppBlurTextureDesc.Format;
		rtvBlurDesc.Texture2D.MipSlice = 0;
		rtvBlurDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		device->CreateRenderTargetView(
			ppBlurTexture.Get(),
			&rtvBlurDesc,
			ppBlurRTV.ReleaseAndGetAddressOf());
		// Create the Shader Resource View
		// By passing it a null description for the SRV, we
		// get a "default" SRV that has access to the entire resource
		device->CreateShaderResourceView(
			ppBlurTexture.Get(),
			0,
			ppBlurSRV.ReleaseAndGetAddressOf());

	}

	//Pixelate Post Processing
	{
		//create post processing texture
		D3D11_TEXTURE2D_DESC ppPixelateTextureDesc = {};
		ppPixelateTextureDesc.Width = windowWidth;
		ppPixelateTextureDesc.Height = windowHeight;
		ppPixelateTextureDesc.ArraySize = 1;
		ppPixelateTextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		ppPixelateTextureDesc.CPUAccessFlags = 0;
		ppPixelateTextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		ppPixelateTextureDesc.MipLevels = 1;
		ppPixelateTextureDesc.MiscFlags = 0;
		ppPixelateTextureDesc.SampleDesc.Count = 1;
		ppPixelateTextureDesc.SampleDesc.Quality = 0;
		ppPixelateTextureDesc.Usage = D3D11_USAGE_DEFAULT;
		// Create the resource (no need to track it after the views are created below)
		Microsoft::WRL::ComPtr<ID3D11Texture2D> ppPixelateTexture;
		device->CreateTexture2D(&ppPixelateTextureDesc, 0, ppPixelateTexture.GetAddressOf());

		// Create the Render Target View
		D3D11_RENDER_TARGET_VIEW_DESC rtvPixelateDesc = {};
		rtvPixelateDesc.Format = ppPixelateTextureDesc.Format;
		rtvPixelateDesc.Texture2D.MipSlice = 0;
		rtvPixelateDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		device->CreateRenderTargetView(
			ppPixelateTexture.Get(),
			&rtvPixelateDesc,
			ppPixelateRTV.ReleaseAndGetAddressOf());
		// Create the Shader Resource View
		// By passing it a null description for the SRV, we
		// get a "default" SRV that has access to the entire resource
		device->CreateShaderResourceView(
			ppPixelateTexture.Get(),
			0,
			ppPixelateSRV.ReleaseAndGetAddressOf());

	}
	
	//Posterize Post Processing
	{
		//create post processing texture
		D3D11_TEXTURE2D_DESC ppPosterizeTextureDesc = {};
		ppPosterizeTextureDesc.Width = windowWidth;
		ppPosterizeTextureDesc.Height = windowHeight;
		ppPosterizeTextureDesc.ArraySize = 1;
		ppPosterizeTextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		ppPosterizeTextureDesc.CPUAccessFlags = 0;
		ppPosterizeTextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		ppPosterizeTextureDesc.MipLevels = 1;
		ppPosterizeTextureDesc.MiscFlags = 0;
		ppPosterizeTextureDesc.SampleDesc.Count = 1;
		ppPosterizeTextureDesc.SampleDesc.Quality = 0;
		ppPosterizeTextureDesc.Usage = D3D11_USAGE_DEFAULT;
		// Create the resource (no need to track it after the views are created below)
		Microsoft::WRL::ComPtr<ID3D11Texture2D> ppPosterizeTexture;
		device->CreateTexture2D(&ppPosterizeTextureDesc, 0, ppPosterizeTexture.GetAddressOf());

		// Create the Render Target View
		D3D11_RENDER_TARGET_VIEW_DESC rtvPosterizeDesc = {};
		rtvPosterizeDesc.Format = ppPosterizeTextureDesc.Format;
		rtvPosterizeDesc.Texture2D.MipSlice = 0;
		rtvPosterizeDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		device->CreateRenderTargetView(
			ppPosterizeTexture.Get(),
			&rtvPosterizeDesc,
			ppPosterizeRTV.ReleaseAndGetAddressOf());
		// Create the Shader Resource View
		// By passing it a null description for the SRV, we
		// get a "default" SRV that has access to the entire resource
		device->CreateShaderResourceView(
			ppPosterizeTexture.Get(),
			0,
			ppPosterizeSRV.ReleaseAndGetAddressOf());

	}

	//create the depth/stencil view for shadow maps
	D3D11_DEPTH_STENCIL_VIEW_DESC shadowDSDesc = {};
	shadowDSDesc.Format = DXGI_FORMAT_D32_FLOAT;
	shadowDSDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	shadowDSDesc.Texture2D.MipSlice = 0;
	device->CreateDepthStencilView(
		shadowTexture.Get(),
		&shadowDSDesc,
		shadowDSV.GetAddressOf());

	//create the shader resource view for the shadow maps
	D3D11_SHADER_RESOURCE_VIEW_DESC shadowSRVDesc = {};
	shadowSRVDesc.Format = DXGI_FORMAT_R32_FLOAT;
	shadowSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shadowSRVDesc.Texture2D.MipLevels = 1;
	shadowSRVDesc.Texture2D.MostDetailedMip = 0;
	device->CreateShaderResourceView(
		shadowTexture.Get(),
		&shadowSRVDesc,
		shadowSRV.GetAddressOf());


	D3D11_SAMPLER_DESC samplerDescription = {};
	samplerDescription.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDescription.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDescription.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDescription.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDescription.MaxAnisotropy = 8;
	samplerDescription.MaxLOD = D3D11_FLOAT32_MAX;

	device.Get()->CreateSamplerState(
		&samplerDescription,
		&samplerState);

	device.Get()->CreateSamplerState(
		&samplerDescription,
		&shadowSampler);

	//fully white specular map
	CreateWICTextureFromFile(device.Get(), FixPath(L"../../Assets/Specular_Maps/fully_specular.png").c_str(), 0, fullySpecularSRV.GetAddressOf());

	//flat normal map
	CreateWICTextureFromFile(device.Get(), FixPath(L"../../Assets/Normal_Maps/flat_normal.png").c_str(), 0, flatNormalSRV.GetAddressOf());

	//rusty metal
	CreateWICTextureFromFile(device.Get(), FixPath(L"../../Assets/Textures/rustymetal.png").c_str(), 0, rustyMetalSRV.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), FixPath(L"../../Assets/Specular_Maps/rustymetal_specular.png").c_str(), 0, rustyMetalSpecularSRV.GetAddressOf());

	//broken tiles
	CreateWICTextureFromFile(device.Get(), FixPath(L"../../Assets/Textures/brokentiles.png").c_str(), 0, brokenTilesSRV.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), FixPath(L"../../Assets/Specular_Maps/brokentiles_specular.png").c_str(), 0, brokenTilesSpecularSRV.GetAddressOf());

	//tiles
	CreateWICTextureFromFile(device.Get(), FixPath(L"../../Assets/Textures/tiles.png").c_str(), 0, tilesSRV.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), FixPath(L"../../Assets/Specular_Maps/tiles_specular.png").c_str(), 0, tilesSpecularSRV.GetAddressOf());

	//cushion
	CreateWICTextureFromFile(device.Get(), FixPath(L"../../Assets/Textures/cushion.png").c_str(), 0, cushionSRV.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), FixPath(L"../../Assets/Normal_Maps/cushion_normals.png").c_str(), 0, cushionNormalSRV.GetAddressOf());

	//rock
	CreateWICTextureFromFile(device.Get(), FixPath(L"../../Assets/Textures/rock.png").c_str(), 0, rockSRV.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), FixPath(L"../../Assets/Normal_Maps/rock_normals.png").c_str(), 0, rockNormalSRV.GetAddressOf());

	//cobblestone
	CreateWICTextureFromFile(device.Get(), FixPath(L"../../Assets/PBR/Albedo/cobblestone_albedo.png").c_str(), 0, cobblestoneSRV.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), FixPath(L"../../Assets/PBR/Metal/cobblestone_metal.png").c_str(), 0, cobblestoneMetalSRV.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), FixPath(L"../../Assets/PBR/Normal/cobblestone_normals.png").c_str(), 0, cobblestoneNormalSRV.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), FixPath(L"../../Assets/PBR/Roughness/cobblestone_roughness.png").c_str(), 0, cobblestoneRoughnessSRV.GetAddressOf());

	//bronze
	CreateWICTextureFromFile(device.Get(), FixPath(L"../../Assets/PBR/Albedo/bronze_albedo.png").c_str(), 0, bronzeSRV.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), FixPath(L"../../Assets/PBR/Metal/bronze_metal.png").c_str(), 0, bronzeMetalSRV.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), FixPath(L"../../Assets/PBR/Normal/bronze_normals.png").c_str(), 0, bronzeNormalSRV.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), FixPath(L"../../Assets/PBR/Roughness/bronze_roughness.png").c_str(), 0, bronzeRoughnessSRV.GetAddressOf());

	//floor
	CreateWICTextureFromFile(device.Get(), FixPath(L"../../Assets/PBR/Albedo/floor_albedo.png").c_str(), 0, floorSRV.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), FixPath(L"../../Assets/PBR/Metal/floor_metal.png").c_str(), 0, floorMetalSRV.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), FixPath(L"../../Assets/PBR/Normal/floor_normals.png").c_str(), 0, floorNormalSRV.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), FixPath(L"../../Assets/PBR/Roughness/floor_roughness.png").c_str(), 0, floorRoughnessSRV.GetAddressOf());

	//paint
	CreateWICTextureFromFile(device.Get(), FixPath(L"../../Assets/PBR/Albedo/paint_albedo.png").c_str(), 0, paintSRV.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), FixPath(L"../../Assets/PBR/Metal/paint_metal.png").c_str(), 0, paintMetalSRV.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), FixPath(L"../../Assets/PBR/Normal/paint_normals.png").c_str(), 0, paintNormalSRV.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), FixPath(L"../../Assets/PBR/Roughness/paint_roughness.png").c_str(), 0, paintRoughnessSRV.GetAddressOf());

	//rough
	CreateWICTextureFromFile(device.Get(), FixPath(L"../../Assets/PBR/Albedo/rough_albedo.png").c_str(), 0, roughSRV.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), FixPath(L"../../Assets/PBR/Metal/rough_metal.png").c_str(), 0, roughMetalSRV.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), FixPath(L"../../Assets/PBR/Normal/rough_normals.png").c_str(), 0, roughNormalSRV.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), FixPath(L"../../Assets/PBR/Roughness/rough_roughness.png").c_str(), 0, roughRoughnessSRV.GetAddressOf());

	//scratched
	CreateWICTextureFromFile(device.Get(), FixPath(L"../../Assets/PBR/Albedo/scratched_albedo.png").c_str(), 0, scratchedSRV.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), FixPath(L"../../Assets/PBR/Metal/scratched_metal.png").c_str(), 0, scratchedMetalSRV.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), FixPath(L"../../Assets/PBR/Normal/scratched_normals.png").c_str(), 0, scratchedNormalSRV.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), FixPath(L"../../Assets/PBR/Roughness/scratched_roughness.png").c_str(), 0, scratchedRoughnessSRV.GetAddressOf());

	//wood
	CreateWICTextureFromFile(device.Get(), FixPath(L"../../Assets/PBR/Albedo/wood_albedo.png").c_str(), 0, woodSRV.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), FixPath(L"../../Assets/PBR/Metal/wood_metal.png").c_str(), 0, woodMetalSRV.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), FixPath(L"../../Assets/PBR/Normal/wood_normals.png").c_str(), 0, woodNormalSRV.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), FixPath(L"../../Assets/PBR/Roughness/wood_roughness.png").c_str(), 0, woodRoughnessSRV.GetAddressOf());




	cloudsBlueSRV = CreateCubemap(
		FixPath(L"../../Assets/Skies/Clouds_Blue/right.png").c_str(),
		FixPath(L"../../Assets/Skies/Clouds_Blue/left.png").c_str(),
		FixPath(L"../../Assets/Skies/Clouds_Blue/up.png").c_str(),
		FixPath(L"../../Assets/Skies/Clouds_Blue/down.png").c_str(),
		FixPath(L"../../Assets/Skies/Clouds_Blue/front.png").c_str(),
		FixPath(L"../../Assets/Skies/Clouds_Blue/back.png").c_str());
	/*
	cloudsPinkSRV = CreateCubemap(
		FixPath(L"../../Assets/Skies/Clouds_Pink/right.png").c_str(),
		FixPath(L"../../Assets/Skies/Clouds_Pink/left.png").c_str(),
		FixPath(L"../../Assets/Skies/Clouds_Pink/up.png").c_str(),
		FixPath(L"../../Assets/Skies/Clouds_Pink/down.png").c_str(),
		FixPath(L"../../Assets/Skies/Clouds_Pink/front.png").c_str(),
		FixPath(L"../../Assets/Skies/Clouds_Pink/back.png").c_str());

	coldSunsetSRV = CreateCubemap(
		FixPath(L"../../Assets/Skies/Cold_Sunset/right.png").c_str(),
		FixPath(L"../../Assets/Skies/Cold_Sunset/left.png").c_str(),
		FixPath(L"../../Assets/Skies/Cold_Sunset/up.png").c_str(),
		FixPath(L"../../Assets/Skies/Cold_Sunset/down.png").c_str(),
		FixPath(L"../../Assets/Skies/Cold_Sunset/front.png").c_str(),
		FixPath(L"../../Assets/Skies/Cold_Sunset/back.png").c_str());

	planetSRV = CreateCubemap(
		FixPath(L"../../Assets/Skies/Planet/right.png").c_str(),
		FixPath(L"../../Assets/Skies/Planet/left.png").c_str(),
		FixPath(L"../../Assets/Skies/Planet/up.png").c_str(),
		FixPath(L"../../Assets/Skies/Planet/down.png").c_str(),
		FixPath(L"../../Assets/Skies/Planet/front.png").c_str(),
		FixPath(L"../../Assets/Skies/Planet/back.png").c_str());
	*/
}


void Game::CreateMaterials()
{
	materials.push_back(Material(DirectX::XMFLOAT4(1, 1, 1, 1), vs, tps, 0.5f, false));
	materials[materials.size() - 1].AddSampler("BasicSampler", samplerState);
	materials[materials.size() - 1].AddTextureSRV("SurfaceTexture", rustyMetalSRV);
	materials[materials.size() - 1].AddTextureSRV("SurfaceTextureSpecular", rustyMetalSpecularSRV);

	materials.push_back(Material(DirectX::XMFLOAT4(1, 1, 1, 1), vs, tps, 0.5f, false));
	materials[materials.size() - 1].AddSampler("BasicSampler", samplerState);
	materials[materials.size() - 1].AddTextureSRV("SurfaceTexture", brokenTilesSRV);
	materials[materials.size() - 1].AddTextureSRV("SurfaceTextureSpecular", brokenTilesSpecularSRV);

	materials.push_back(Material(DirectX::XMFLOAT4(1, 1, 1, 1), vs, tps, 0.5f, false));
	materials[materials.size() - 1].AddSampler("BasicSampler", samplerState);
	materials[materials.size() - 1].AddTextureSRV("SurfaceTexture", tilesSRV);
	materials[materials.size() - 1].AddTextureSRV("SurfaceTextureSpecular", tilesSpecularSRV);

	materials.push_back(Material(DirectX::XMFLOAT4(1, 1, 1, 1), nvs, nps, 0.5f, false));
	materials[materials.size() - 1].AddSampler("BasicSampler", samplerState);
	materials[materials.size() - 1].AddTextureSRV("SurfaceTexture", cushionSRV);
	materials[materials.size() - 1].AddTextureSRV("SurfaceTextureSpecular", fullySpecularSRV);
	materials[materials.size() - 1].AddTextureSRV("SurfaceTextureNormal", cushionNormalSRV);


	materials.push_back(Material(DirectX::XMFLOAT4(1, 1, 1, 1), nvs, nps, 0.5f, false));
	materials[materials.size() - 1].AddSampler("BasicSampler", samplerState);
	materials[materials.size() - 1].AddTextureSRV("SurfaceTexture", rockSRV);
	materials[materials.size() - 1].AddTextureSRV("SurfaceTextureSpecular", fullySpecularSRV);
	materials[materials.size() - 1].AddTextureSRV("SurfaceTextureNormal", rockNormalSRV);

	materials.push_back(Material(DirectX::XMFLOAT4(1, 1, 1, 1), nvs, PBRps, 0.5f, true));
	materials[materials.size() - 1].AddSampler("BasicSampler", samplerState);
	materials[materials.size() - 1].AddTextureSRV("Albedo", cobblestoneSRV);
	materials[materials.size() - 1].AddTextureSRV("MetalnessMap", cobblestoneMetalSRV);
	materials[materials.size() - 1].AddTextureSRV("NormalMap", cobblestoneNormalSRV);
	materials[materials.size() - 1].AddTextureSRV("RoughnessMap", cobblestoneRoughnessSRV);

	materials.push_back(Material(DirectX::XMFLOAT4(1, 1, 1, 1), nvs, PBRps, 0.5f, true));
	materials[materials.size() - 1].AddSampler("BasicSampler", samplerState);
	materials[materials.size() - 1].AddTextureSRV("Albedo", bronzeSRV);
	materials[materials.size() - 1].AddTextureSRV("MetalnessMap", bronzeMetalSRV);
	materials[materials.size() - 1].AddTextureSRV("NormalMap", bronzeNormalSRV);
	materials[materials.size() - 1].AddTextureSRV("RoughnessMap", bronzeRoughnessSRV);

	materials.push_back(Material(DirectX::XMFLOAT4(1, 1, 1, 1), nvs, PBRps, 0.5f, true));
	materials[materials.size() - 1].AddSampler("BasicSampler", samplerState);
	materials[materials.size() - 1].AddTextureSRV("Albedo", floorSRV);
	materials[materials.size() - 1].AddTextureSRV("MetalnessMap", floorMetalSRV);
	materials[materials.size() - 1].AddTextureSRV("NormalMap", floorNormalSRV);
	materials[materials.size() - 1].AddTextureSRV("RoughnessMap", floorRoughnessSRV);

	materials.push_back(Material(DirectX::XMFLOAT4(1, 1, 1, 1), nvs, PBRps, 0.5f, true));
	materials[materials.size() - 1].AddSampler("BasicSampler", samplerState);
	materials[materials.size() - 1].AddTextureSRV("Albedo", paintSRV);
	materials[materials.size() - 1].AddTextureSRV("MetalnessMap", paintMetalSRV);
	materials[materials.size() - 1].AddTextureSRV("NormalMap", paintNormalSRV);
	materials[materials.size() - 1].AddTextureSRV("RoughnessMap", paintRoughnessSRV);

	materials.push_back(Material(DirectX::XMFLOAT4(1, 1, 1, 1), nvs, PBRps, 0.5f, true));
	materials[materials.size() - 1].AddSampler("BasicSampler", samplerState);
	materials[materials.size() - 1].AddTextureSRV("Albedo", roughSRV);
	materials[materials.size() - 1].AddTextureSRV("MetalnessMap", roughMetalSRV);
	materials[materials.size() - 1].AddTextureSRV("NormalMap", roughNormalSRV);
	materials[materials.size() - 1].AddTextureSRV("RoughnessMap", roughRoughnessSRV);

	materials.push_back(Material(DirectX::XMFLOAT4(1, 1, 1, 1), nvs, PBRps, 0.5f, true));
	materials[materials.size() - 1].AddSampler("BasicSampler", samplerState);
	materials[materials.size() - 1].AddTextureSRV("Albedo", scratchedSRV);
	materials[materials.size() - 1].AddTextureSRV("MetalnessMap", scratchedMetalSRV);
	materials[materials.size() - 1].AddTextureSRV("NormalMap", scratchedNormalSRV);
	materials[materials.size() - 1].AddTextureSRV("RoughnessMap", scratchedRoughnessSRV);

	materials.push_back(Material(DirectX::XMFLOAT4(1, 1, 1, 1), nvs, PBRps, 0.5f, true));
	materials[materials.size() - 1].AddSampler("BasicSampler", samplerState);
	materials[materials.size() - 1].AddTextureSRV("Albedo", woodSRV);
	materials[materials.size() - 1].AddTextureSRV("MetalnessMap", woodMetalSRV);
	materials[materials.size() - 1].AddTextureSRV("NormalMap", woodNormalSRV);
	materials[materials.size() - 1].AddTextureSRV("RoughnessMap", woodRoughnessSRV);
}

void Game::CreateLights()
{
	float lightProjectionSize = 50.0f;
	Light light = {};
	light.color = XMFLOAT3(0.5f, 0.5f, 0.5f);
	light.direction = XMFLOAT3(0.1f, -1, 0.1f);
	light.intensity = .5f;
	light.type = 0;
	lights.push_back(light);
	//generate light view matrix
	XMMATRIX lightView = XMMatrixLookToLH(
		-XMLoadFloat3(&light.direction) * 20, // Position: "Backing up" 20 units from origin
		XMLoadFloat3(&light.direction), // Direction: light's direction
		XMVectorSet(0, 1, 0, 0)); // Up: World up vector (Y axis)
	lightViewMatrix = lightView;
	//create light projection matrix
	XMMATRIX lightProjection = XMMatrixOrthographicLH(
		lightProjectionSize,
		lightProjectionSize,
		1.0f,
		100.0f);
	lightProjectionMatrix = lightProjection;

	light = {};
	light.color = XMFLOAT3(0, 0, 0);
	light.direction = XMFLOAT3(0, -1, 0);
	light.intensity = .5f;
	light.type = 0;
	lights.push_back(light);


	light = {};
	light.color = XMFLOAT3(0, 0, 0);
	light.direction = XMFLOAT3(1, 0, 0);
	light.intensity = .5f;
	light.type = 0;
	lights.push_back(light);


	light = {};
	light.color = XMFLOAT3(0, 1, 0);
	light.position = XMFLOAT3(5, 0, 0);
	light.intensity = .5f;
	light.range = 5.0f;
	light.type = 1;
	lights.push_back(light);


	light = {};
	light.color = XMFLOAT3(1, 1, 1);
	light.position = XMFLOAT3(-5, 0, 0);
	light.intensity = 1.0f;
	light.range = 10.0f;
	light.type = 1;
	lights.push_back(light);

}


// --------------------------------------------------------
// Creates the geometry we're going to draw - a single triangle for now
// --------------------------------------------------------
void Game::CreateGeometry()
{
	//clean up empty meshes before reassignment
	delete[] meshes;
	meshCount = 7;
	entityCount = 5;
	meshes = new std::shared_ptr<Mesh>[meshCount];
	delete[] entities;
	entities = new std::shared_ptr<GameEntity>[entityCount];

	delete[] entityPositions;
	entityPositions = new DirectX::XMFLOAT3[entityCount];

	delete[] entityRotations;
	entityRotations = new DirectX::XMFLOAT3[entityCount];

	delete[] entityScales;
	entityScales = new DirectX::XMFLOAT3[entityCount];

	// Create some temporary variables to represent colors
	// - Not necessary, just makes things more readable
	XMFLOAT4 red = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	XMFLOAT4 green = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	XMFLOAT4 blue = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	XMFLOAT4 black = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	XMFLOAT4 white = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	XMFLOAT3 baseNormal = XMFLOAT3(0, 0, -1);
	XMFLOAT2 baseUV = XMFLOAT2(0, 0);
	/*
	//Triangle
	{
		Vertex vertices[] =
		{
			{ XMFLOAT3(+0.0f, +0.5f, +0.0f), baseNormal, baseUV },
			{ XMFLOAT3(+0.5f, -0.5f, +0.0f), baseNormal, baseUV },
			{ XMFLOAT3(-0.5f, -0.5f, +0.0f), baseNormal, baseUV },
		};
		// Set up indices, which tell us which vertices to use and in which order
		// - This is redundant for just 3 vertices, but will be more useful later
		// - Indices are technically not required if the vertices are in the buffer 
		//    in the correct order and each one will be used exactly once
		// - But just to see how it's done...
		unsigned int indices[] = { 0, 1, 2 };
		std::shared_ptr<Mesh> triangle; // Declaration (probably in a header)
		triangle = std::make_shared<Mesh>(device, vertices, 3, indices, 3); // Initialization

		meshes[0] = triangle;
	}
	//Square
	{
		Vertex vertices[] =
		{
			{ XMFLOAT3(-0.5f, +0.7f, +0.0f), baseNormal, baseUV },
			{ XMFLOAT3(-0.2f, +0.7f, +0.0f), baseNormal, baseUV },
			{ XMFLOAT3(-0.2f, +0.2f, +0.0f), baseNormal, baseUV },
			{ XMFLOAT3(-0.5f, +0.7f, +0.0f), baseNormal, baseUV },
			{ XMFLOAT3(-0.2f, +0.2f, +0.0f), baseNormal, baseUV },
			{ XMFLOAT3(-0.5f, +0.2f, +0.0f), baseNormal, baseUV }
		};
		// Set up indices, which tell us which vertices to use and in which order
		// - This is redundant for just 3 vertices, but will be more useful later
		// - Indices are technically not required if the vertices are in the buffer 
		//    in the correct order and each one will be used exactly once
		// - But just to see how it's done...
		unsigned int indices[] = { 0, 1, 2, 3, 4, 5 };
		std::shared_ptr<Mesh> box; // Declaration (probably in a header)
		box = std::make_shared<Mesh>(device, vertices, 6, indices, 6); // Initialization

		meshes[1] = box;
	}
	//Butterfly
	{
		Vertex vertices[] =
		{
			{ XMFLOAT3(+0.5f, +0.7f, +0.0f), baseNormal, baseUV },
			{ XMFLOAT3(+0.55f, +0.5f, +0.0f), baseNormal, baseUV },
			{ XMFLOAT3(+0.35f, +0.5f, +0.0f), baseNormal, baseUV },
			{ XMFLOAT3(+0.6f, +0.7f, +0.0f), baseNormal, baseUV },
			{ XMFLOAT3(+0.75f, +0.5f, +0.0f), baseNormal, baseUV },
			{ XMFLOAT3(+0.55f, +0.5f, +0.0f), baseNormal, baseUV },
			{ XMFLOAT3(+0.35f, +0.5f, +0.0f), baseNormal, baseUV },
			{ XMFLOAT3(+0.55f, +0.5f, +0.0f), baseNormal, baseUV },
			{ XMFLOAT3(+0.45f, +0.0f, +0.0f), baseNormal, baseUV },
			{ XMFLOAT3(+0.55f, +0.5f, +0.0f), baseNormal, baseUV },
			{ XMFLOAT3(+0.75f, +0.5f, +0.0f), baseNormal, baseUV },
			{ XMFLOAT3(+0.65f, +0.0f, +0.0f), baseNormal, baseUV },
		};
		// Set up indices, which tell us which vertices to use and in which order
		// - This is redundant for just 3 vertices, but will be more useful later
		// - Indices are technically not required if the vertices are in the buffer 
		//    in the correct order and each one will be used exactly once
		// - But just to see how it's done...
		unsigned int indices[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
		std::shared_ptr<Mesh> butterfly; // Declaration (probably in a header)
		butterfly = std::make_shared<Mesh>(device, vertices, 12, indices, 12); // Initialization

		meshes[2] = butterfly;
	}
	//Cube
	{
		Vertex vertices[] =
		{
			{ XMFLOAT3(+0.5f, +0.5f, -0.5f), baseNormal, baseUV },
			{ XMFLOAT3(+0.5f, -0.5f, -0.5f), baseNormal, baseUV },
			{ XMFLOAT3(-0.5f, -0.5f, -0.5f), baseNormal, baseUV },
			{ XMFLOAT3(-0.5f, -0.5f, -0.5f), baseNormal, baseUV },
			{ XMFLOAT3(-0.5f, +0.5f, -0.5f), baseNormal, baseUV },
			{ XMFLOAT3(+0.5f, +0.5f, -0.5f), baseNormal, baseUV },

			{ XMFLOAT3(+0.5f, +0.5f, -0.5f), baseNormal, baseUV },
			{ XMFLOAT3(+0.5f, +0.5f, +0.5f), baseNormal, baseUV },
			{ XMFLOAT3(+0.5f, -0.5f, -0.5f), baseNormal, baseUV },
			{ XMFLOAT3(+0.5f, +0.5f, +0.5f), baseNormal, baseUV },
			{ XMFLOAT3(+0.5f, -0.5f, +0.5f), baseNormal, baseUV },
			{ XMFLOAT3(+0.5f, -0.5f, -0.5f), baseNormal, baseUV },


			{ XMFLOAT3(-0.5f, -0.5f, -0.5f), baseNormal, baseUV },
			{ XMFLOAT3(-0.5f, +0.5f, +0.5f), baseNormal, baseUV },
			{ XMFLOAT3(-0.5f, +0.5f, -0.5f), baseNormal, baseUV },
			{ XMFLOAT3(-0.5f, -0.5f, -0.5f), baseNormal, baseUV },
			{ XMFLOAT3(-0.5f, -0.5f, +0.5f), baseNormal, baseUV },
			{ XMFLOAT3(-0.5f, +0.5f, +0.5f), baseNormal, baseUV },

			{ XMFLOAT3(-0.5f, -0.5f, +0.5f), baseNormal, baseUV },
			{ XMFLOAT3(+0.5f, -0.5f, +0.5f), baseNormal, baseUV },
			{ XMFLOAT3(+0.5f, +0.5f, +0.5f), baseNormal, baseUV },
			{ XMFLOAT3(+0.5f, +0.5f, +0.5f), baseNormal, baseUV },
			{ XMFLOAT3(-0.5f, +0.5f, +0.5f), baseNormal, baseUV },
			{ XMFLOAT3(-0.5f, -0.5f, +0.5f), baseNormal, baseUV },

			{ XMFLOAT3(-0.5f, +0.5f, +0.5f), baseNormal, baseUV },
			{ XMFLOAT3(+0.5f, +0.5f, -0.5f), baseNormal, baseUV },
			{ XMFLOAT3(-0.5f, +0.5f, -0.5f), baseNormal, baseUV },
			{ XMFLOAT3(-0.5f, +0.5f, +0.5f), baseNormal, baseUV },
			{ XMFLOAT3(+0.5f, +0.5f, +0.5f), baseNormal, baseUV },
			{ XMFLOAT3(+0.5f, +0.5f, -0.5f), baseNormal, baseUV },

			{ XMFLOAT3(-0.5f, -0.5f, -0.5f), baseNormal, baseUV },
			{ XMFLOAT3(+0.5f, -0.5f, -0.5f), baseNormal, baseUV },
			{ XMFLOAT3(-0.5f, -0.5f, +0.5f), baseNormal, baseUV },
			{ XMFLOAT3(+0.5f, -0.5f, -0.5f), baseNormal, baseUV },
			{ XMFLOAT3(+0.5f, -0.5f, +0.5f), baseNormal, baseUV },
			{ XMFLOAT3(-0.5f, -0.5f, +0.5f), baseNormal, baseUV },
		};

		// Set up indices, which tell us which vertices to use and in which order
		// - This is redundant for just 3 vertices, but will be more useful later
		// - Indices are technically not required if the vertices are in the buffer 
		//    in the correct order and each one will be used exactly once
		// - But just to see how it's done...
		unsigned int indices[36];
		for (int i = 0; i < 36; i++)
		{
			indices[i] = i;
		}
		std::shared_ptr<Mesh> cube; // Declaration (probably in a header)
		cube = std::make_shared<Mesh>(device, vertices, 36, indices, 36); // Initialization

		meshes[3] = cube;
	}
	*/
	meshes[0] = std::make_shared<Mesh>(device, FixPath(L"../../Assets/Models/sphere.obj").c_str());
	meshes[1] = std::make_shared<Mesh>(device, FixPath(L"../../Assets/Models/cube.obj").c_str());
	meshes[2] = std::make_shared<Mesh>(device, FixPath(L"../../Assets/Models/cylinder.obj").c_str());
	meshes[3] = std::make_shared<Mesh>(device, FixPath(L"../../Assets/Models/helix.obj").c_str());
	meshes[4] = std::make_shared<Mesh>(device, FixPath(L"../../Assets/Models/quad.obj").c_str());
	meshes[5] = std::make_shared<Mesh>(device, FixPath(L"../../Assets/Models/quad_double_sided.obj").c_str());
	meshes[6] = std::make_shared<Mesh>(device, FixPath(L"../../Assets/Models/torus.obj").c_str());
	
	entities[0] = std::make_shared<GameEntity>(meshes[0], std::make_shared<Material>(materials[8]));
	entities[1] = std::make_shared<GameEntity>(meshes[6], std::make_shared<Material>(materials[5]));
	entities[2] = std::make_shared<GameEntity>(meshes[1], std::make_shared<Material>(materials[6]));
	entities[3] = std::make_shared<GameEntity>(meshes[3], std::make_shared<Material>(materials[7]));
	

	for (int i = 0; i < 4; i++)
	{
		entities[i]->GetTransform()->SetPosition(DirectX::XMFLOAT3(-7.5f + 5 * i, 0, 5));
		entityPositions[i] = entities[i]->GetTransform()->GetPosition();
	}

	entities[4] = std::make_shared<GameEntity>(meshes[5], std::make_shared<Material>(materials[11]));
	entities[4]->GetTransform()->SetPosition(DirectX::XMFLOAT3(0, -2.5f, 0));
	entities[4]->GetTransform()->SetScale(DirectX::XMFLOAT3(20, 20, 20));

	//create Skybox
	sky = std::make_shared<Sky>(meshes[1], samplerState, device, skyVS, skyPS);
	sky->SetShaderResourceView(cloudsBlueSRV);

}


// --------------------------------------------------------
// Handle resizing to match the new window size.
//  - DXCore needs to resize the back buffer
//  - Eventually, we'll want to update our 3D camera
// --------------------------------------------------------
void Game::OnResize()
{
	// Handle base-level DX resize stuff
	DXCore::OnResize();
	for (unsigned int i = 0; i < numCameras; i++)
		cameras[i]->UpdateProjectionMatrix(float(windowWidth) / float(windowHeight));

	ppBlurSRV.Reset();
	ppBlurRTV.Reset();

	//create post processing texture
	D3D11_TEXTURE2D_DESC ppBlurTextureDesc = {};
	ppBlurTextureDesc.Width = windowWidth;
	ppBlurTextureDesc.Height = windowHeight;
	ppBlurTextureDesc.ArraySize = 1;
	ppBlurTextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	ppBlurTextureDesc.CPUAccessFlags = 0;
	ppBlurTextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	ppBlurTextureDesc.MipLevels = 1;
	ppBlurTextureDesc.MiscFlags = 0;
	ppBlurTextureDesc.SampleDesc.Count = 1;
	ppBlurTextureDesc.SampleDesc.Quality = 0;
	ppBlurTextureDesc.Usage = D3D11_USAGE_DEFAULT;
	// Create the resource (no need to track it after the views are created below)
	Microsoft::WRL::ComPtr<ID3D11Texture2D> ppBlurTexture;
	device->CreateTexture2D(&ppBlurTextureDesc, 0, ppBlurTexture.GetAddressOf());

	// Create the Render Target View
	D3D11_RENDER_TARGET_VIEW_DESC rtvBlurDesc = {};
	rtvBlurDesc.Format = ppBlurTextureDesc.Format;
	rtvBlurDesc.Texture2D.MipSlice = 0;
	rtvBlurDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	device->CreateRenderTargetView(
		ppBlurTexture.Get(),
		&rtvBlurDesc,
		ppBlurRTV.ReleaseAndGetAddressOf());
	// Create the Shader Resource View
	// By passing it a null description for the SRV, we
	// get a "default" SRV that has access to the entire resource
	device->CreateShaderResourceView(
		ppBlurTexture.Get(),
		0,
		ppBlurSRV.ReleaseAndGetAddressOf());


	ppPixelateRTV.Reset();
	ppPixelateSRV.Reset();
	//create post processing texture
	D3D11_TEXTURE2D_DESC ppPixelateTextureDesc = {};
	ppPixelateTextureDesc.Width = windowWidth;
	ppPixelateTextureDesc.Height = windowHeight;
	ppPixelateTextureDesc.ArraySize = 1;
	ppPixelateTextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	ppPixelateTextureDesc.CPUAccessFlags = 0;
	ppPixelateTextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	ppPixelateTextureDesc.MipLevels = 1;
	ppPixelateTextureDesc.MiscFlags = 0;
	ppPixelateTextureDesc.SampleDesc.Count = 1;
	ppPixelateTextureDesc.SampleDesc.Quality = 0;
	ppPixelateTextureDesc.Usage = D3D11_USAGE_DEFAULT;
	// Create the resource (no need to track it after the views are created below)
	Microsoft::WRL::ComPtr<ID3D11Texture2D> ppPixelateTexture;
	device->CreateTexture2D(&ppPixelateTextureDesc, 0, ppPixelateTexture.GetAddressOf());

	// Create the Render Target View
	D3D11_RENDER_TARGET_VIEW_DESC rtvPixelateDesc = {};
	rtvPixelateDesc.Format = ppPixelateTextureDesc.Format;
	rtvPixelateDesc.Texture2D.MipSlice = 0;
	rtvPixelateDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	device->CreateRenderTargetView(
		ppPixelateTexture.Get(),
		&rtvPixelateDesc,
		ppPixelateRTV.ReleaseAndGetAddressOf());
	// Create the Shader Resource View
	// By passing it a null description for the SRV, we
	// get a "default" SRV that has access to the entire resource
	device->CreateShaderResourceView(
		ppPixelateTexture.Get(),
		0,
		ppPixelateSRV.ReleaseAndGetAddressOf());

	ppPosterizeRTV.Reset();
	ppPosterizeSRV.Reset();

	//create post processing texture
	D3D11_TEXTURE2D_DESC ppPosterizeTextureDesc = {};
	ppPosterizeTextureDesc.Width = windowWidth;
	ppPosterizeTextureDesc.Height = windowHeight;
	ppPosterizeTextureDesc.ArraySize = 1;
	ppPosterizeTextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	ppPosterizeTextureDesc.CPUAccessFlags = 0;
	ppPosterizeTextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	ppPosterizeTextureDesc.MipLevels = 1;
	ppPosterizeTextureDesc.MiscFlags = 0;
	ppPosterizeTextureDesc.SampleDesc.Count = 1;
	ppPosterizeTextureDesc.SampleDesc.Quality = 0;
	ppPosterizeTextureDesc.Usage = D3D11_USAGE_DEFAULT;
	// Create the resource (no need to track it after the views are created below)
	Microsoft::WRL::ComPtr<ID3D11Texture2D> ppPosterizeTexture;
	device->CreateTexture2D(&ppPosterizeTextureDesc, 0, ppPosterizeTexture.GetAddressOf());

	// Create the Render Target View
	D3D11_RENDER_TARGET_VIEW_DESC rtvPosterizeDesc = {};
	rtvPosterizeDesc.Format = ppPosterizeTextureDesc.Format;
	rtvPosterizeDesc.Texture2D.MipSlice = 0;
	rtvPosterizeDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	device->CreateRenderTargetView(
		ppPosterizeTexture.Get(),
		&rtvPosterizeDesc,
		ppPosterizeRTV.ReleaseAndGetAddressOf());
	// Create the Shader Resource View
	// By passing it a null description for the SRV, we
	// get a "default" SRV that has access to the entire resource
	device->CreateShaderResourceView(
		ppPosterizeTexture.Get(),
		0,
		ppPosterizeSRV.ReleaseAndGetAddressOf());

}

// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{

	UpdateImGui(deltaTime);
	// Example input checking: Quit if the escape key is pressed
	if (Input::GetInstance().KeyDown(VK_ESCAPE))
		Quit();

	BuildUi();
	//update shader camera values

	cameras[activeCameraIndex]->UpdateViewMatrix();
	cameras[activeCameraIndex]->UpdateProjectionMatrix(float(windowWidth) / float(windowHeight));
	cameras[activeCameraIndex]->Update(deltaTime);

	entities[0]->GetTransform()->SetPosition(DirectX::XMFLOAT3(entities[0]->GetTransform()->GetPosition().x, sin(totalTime), 5));
	entities[1]->GetTransform()->SetRotation(0, 0, totalTime);
	entities[2]->GetTransform()->SetPosition(DirectX::XMFLOAT3(entities[2]->GetTransform()->GetPosition().x, 0, 5 +  sin(totalTime)));
	entities[3]->GetTransform()->SetRotation(0, totalTime, 0);

	/*
		//When using DirectXMath, need to:
	//1: Load existing data from storage to math types
	XMVECTOR offsetVec = XMLoadFloat3(&vsData.offset);
	XMVECTOR dtVec = XMVectorSet(deltaTime, 0, 0, 0);

	//2: Perform any and all math on the Math types
	offsetVec = XMVectorAdd(offsetVec, dtVec);

	//3: Store the math type data back to a storage type
	XMStoreFloat3(&vsData.offset, offsetVec);

	XMMATRIX world = XMLoadFloat4x4(&vsData.world);
	XMMATRIX rot = XMMatrixRotationZ(deltaTime);
	XMMATRIX sc = XMMatrixScaling(sin(totalTime), sin(totalTime), sin(totalTime));

	//world = XMMatrixMultiply(world, rot);
	//
	//this is less performant than the xmmatrixmultiply
	world = world * sc * rot;

	XMStoreFloat4x4(&vsData.world, world);
	*/
}

// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime)
{
	// Frame START
	// - These things should happen ONCE PER FRAME
	// - At the beginning of Game::Draw() before drawing *anything*
	{
		// Clear the back buffer (erases what's on the screen)
		context->ClearRenderTargetView(backBufferRTV.Get(), bgColor);

		context->ClearRenderTargetView(ppBlurRTV.Get(), bgColor);
		context->ClearRenderTargetView(ppPixelateRTV.Get(), bgColor);
		context->ClearRenderTargetView(ppPosterizeRTV.Get(), bgColor);

		// Clear the depth buffer (resets per-pixel occlusion information)
		context->ClearDepthStencilView(depthBufferDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	}

	//clear the shadow map depth stencil view
	context->ClearDepthStencilView(shadowDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	//set the shadow map as the depth buffer and unbind back buffer
	ID3D11RenderTargetView* nullRTV{};
	context->OMSetRenderTargets(1, &nullRTV, shadowDSV.Get());

	//deactivate pixel shader
	context->PSSetShader(0, 0, 0);
	
	//set viewport to match the resolution of the shadow map
	D3D11_VIEWPORT viewport = {};
	viewport.Width = (float)shadowMapResolution;
	viewport.Height = (float)shadowMapResolution;
	viewport.MaxDepth = 1.0f;
	context->RSSetViewports(1, &viewport);

	//set the shadow map vertex shader to be active
	shadowVS->SetShader();

	//store the view matrix for this light
	XMFLOAT4X4 viewMatrix;
	XMStoreFloat4x4(&viewMatrix, lightViewMatrix);
	shadowVS->SetMatrix4x4("view", viewMatrix);

	//store the projection matrix for this light
	XMFLOAT4X4 projMatrix;
	XMStoreFloat4x4(&projMatrix, lightProjectionMatrix);
	shadowVS->SetMatrix4x4("projection", projMatrix);

	//change the rasterizer state
	context->RSSetState(shadowRasterizer.Get());

	PBRps->SetSamplerState("ShadowSampler", shadowSampler);

	//loop through and create shadow map
	for (unsigned int i = 0; i < entityCount; i++)
	{
		//set vertex shader data
		shadowVS->SetMatrix4x4("world", entities[i]->GetTransform()->GetWorldMatrix());
		shadowVS->CopyAllBufferData();

		//draw the entities through the mesh to avoid resetting shaders and materials
		entities[i]->GetMesh()->Draw(context);
	}

	//reset the pipeline
	viewport.Width = (float)this->windowWidth;
	viewport.Height = (float)this->windowHeight;
	context->RSSetViewports(1, &viewport);

	//disable rasterizer state
	context->RSSetState(0);
	
	if (blurRadius > 0)
	{
		//set the the blur post processing render target
		context->OMSetRenderTargets(1, ppBlurRTV.GetAddressOf(), depthBufferDSV.Get());
	}
	else if (pixelSize > 1)
	{
		//set the render target to be the pixelate rtv
		context->OMSetRenderTargets(1, ppPixelateRTV.GetAddressOf(), depthBufferDSV.Get());
	}
	else if (posterize)
	{
		//set the render target to be the posterize rtv
		context->OMSetRenderTargets(1, ppPosterizeRTV.GetAddressOf(), depthBufferDSV.Get());
	}
	else
	{
		//reset the render target to be the back buffer
		context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthBufferDSV.Get());
	}
	

	nvs->SetMatrix4x4("lightViewMatrix", viewMatrix);
	nvs->SetMatrix4x4("lightProjMatrix", projMatrix);
	PBRps->SetShaderResourceView("ShadowMap", shadowSRV);
	//loop through list of entities drawing each
	for (unsigned int i = 0; i < entityCount; i++)
	{
		entities[i]->GetMaterial()->GetVertexShader()->SetMatrix4x4("viewMatrix", cameras[activeCameraIndex]->GetView());
		entities[i]->GetMaterial()->GetVertexShader()->SetMatrix4x4("projectionMatrix", cameras[activeCameraIndex]->GetProjection());

		entities[i]->GetMaterial()->GetPixelShader()->SetData("lights", &lights[0], sizeof(Light) * lights.size());
		entities[i]->GetMaterial()->GetPixelShader()->SetInt("numLights", lights.size());
		if (!entities[i]->GetMaterial()->GetPBR())
		{
			entities[i]->GetMaterial()->GetPixelShader()->SetFloat3("ambientColor", ambientColor);
		}

		entities[i]->Draw(context, cameras[activeCameraIndex], totalTime);
	}
	
	sky->Draw(context, cameras[activeCameraIndex]);

	ppVS->SetShader();

	if (blurRadius > 0)
	{
		if (pixelSize > 1)
		{
			//set the render target to be the pixelate rtv
			context->OMSetRenderTargets(1, ppPixelateRTV.GetAddressOf(), 0);
		}
		else if (posterize)
		{
			//set the render target to be the pixelate rtv
			context->OMSetRenderTargets(1, ppPosterizeRTV.GetAddressOf(), 0);
		}
		else
		{
			//reset the render target to be the back buffer
			context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), 0);
		}

		// Activate shaders and bind resources
		// Also set any required cbuffer data (not shown)
		ppBlurPS->SetShader();
		ppBlurPS->SetShaderResourceView("Pixels", ppBlurSRV.Get());
		ppBlurPS->SetSamplerState("Sampler", ppSampler.Get());
		ppBlurPS->SetInt("blurRadius", blurRadius);
		ppBlurPS->SetFloat("pixelWidth", static_cast<float>(1.0f / windowWidth));
		ppBlurPS->SetFloat("pixelHeight", static_cast<float>(1.0f / windowHeight));

		ppBlurPS->CopyAllBufferData();

		context->Draw(3, 0); // Draw exactly 3 vertices (one triangle)
	}
	if (pixelSize > 1)
	{
		if (posterize)
		{
			//set the render target to be the pixelate rtv
			context->OMSetRenderTargets(1, ppPosterizeRTV.GetAddressOf(), 0);
		}
		else
		{
			//reset the render target to be the back buffer
			context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), 0);
		}

		ppPixelatePS->SetShader();
		ppPixelatePS->SetShaderResourceView("Pixels", ppPixelateSRV.Get());
		ppPixelatePS->SetSamplerState("Sampler", ppSampler.Get());
		ppPixelatePS->SetInt("pixelSize", pixelSize);
		ppPixelatePS->SetInt("textureWidth", windowWidth);
		ppPixelatePS->SetInt("textureHeight", windowHeight);
		
		ppPixelatePS->CopyAllBufferData();

		context->Draw(3, 0);
	}
	if (posterize)
	{
		//reset the render target to be the back buffer
		context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), 0);

		ppPosterizePS->SetShader();
		ppPosterizePS->SetShaderResourceView("Pixels", ppPosterizeSRV.Get());
		ppPosterizePS->SetSamplerState("Sampler", ppSampler.Get());
		ppPosterizePS->SetFloat("levels", posterizeLevel);

		ppPosterizePS->CopyAllBufferData();

		context->Draw(3, 0);
	}




	// Frame END
	// - These should happen exactly ONCE PER FRAME
	// - At the very end of the frame (after drawing *everything*)
	{
		// Present the back buffer to the user
		//  - Puts the results of what we've drawn onto the window
		//  - Without this, the user never sees anything
		bool vsyncNecessary = vsync || !deviceSupportsTearing || isFullscreen;

		ImGui::Render(); // Turns this frame�s UI into renderable triangles
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData()); // Draws it to the screen

		swapChain->Present(
			vsyncNecessary ? 1 : 0,
			vsyncNecessary ? 0 : DXGI_PRESENT_ALLOW_TEARING);

		// Must re-bind buffers after presenting, as they become unbound
		context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthBufferDSV.Get());
	}

	ID3D11ShaderResourceView* nullSRVs[128] = {};
	context->PSSetShaderResources(0, 128, nullSRVs);
}

void Game::UpdateImGui(float deltaTime)
{
	// Feed fresh data to ImGui
	ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = deltaTime;
	io.DisplaySize.x = (float)this->windowWidth;
	io.DisplaySize.y = (float)this->windowHeight;
	// Reset the frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	// Determine new input capture
	Input& input = Input::GetInstance();
	input.SetKeyboardCapture(io.WantCaptureKeyboard);
	input.SetMouseCapture(io.WantCaptureMouse);
	// Show the demo window
	if (showWindow)
		ImGui::ShowDemoWindow();
}

void Game::BuildUi()
{
	ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_Once);
	ImGui::Begin("Inspector");
	if (ImGui::CollapsingHeader("App Details"))
	{
		ImGui::Text("Framerate: %f fps", ImGui::GetIO().Framerate);
		ImGui::Text("Window Resolution: %dx%d", windowWidth, windowHeight);
		ImGui::ColorEdit4("Background Color", bgColor);
		if (ImGui::Button("Show Demo Window"))
		{
			showWindow = !showWindow;
		}
		ImGui::InputText("New Window Name", nextWindowTitle, IM_ARRAYSIZE(nextWindowTitle));
		if (ImGui::Button("Add A New Window (Max 10)"))
		{
			if (windowsToCreate < 10)
			{
				strcpy_s(windowTitles[windowsToCreate], nextWindowTitle);
				windowsToCreate++;
				nextWindowTitle[0] = '\0';
			}
		}
		for (int i = 0; i < windowsToCreate; i++)
		{
			ImGui::SetNextWindowPos(ImVec2(100.0f, 100.0f + static_cast<float>(i) * 20.0f), ImGuiCond_Always);
			ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiCond_Always);
			if (strlen(windowTitles[i]) > 0)
			{
				ImGui::Begin(windowTitles[i]);
			}
			else
			{
				ImGui::Begin(("New Window " + std::to_string(i + 1)).c_str());
			}
			ImGui::Text("I'm a window!");
			ImGui::End();
		}
	}
	if (ImGui::CollapsingHeader("Meshes"))
	{
		for (unsigned int i = 0; i < meshCount; i++)
		{
			ImGui::Text("Mesh %d: %d triangle(s)", i, meshes[i]->GetIndexCount() / 3);
		}
	}
	if (ImGui::CollapsingHeader("Edit Entity Values"))
	{
		for (unsigned int i = 0; i < entityCount; i++)
		{

			if (ImGui::TreeNode(("Entity " + std::to_string(i + 1)).c_str()))
			{
				entityPositions[i] = entities[i]->GetTransform()->GetPosition();

				ImGui::DragFloat3("Position", reinterpret_cast<float*>(&entityPositions[i]), 0.1f);

				entities[i]->GetTransform()->SetPosition(entityPositions[i]);

				entityRotations[i] = entities[i]->GetTransform()->GetPitchYawRoll();

				ImGui::DragFloat3("Rotation", reinterpret_cast<float*>(&entityRotations[i]), 0.1f);

				entities[i]->GetTransform()->SetRotation(entityRotations[i]);

				entityScales[i] = entities[i]->GetTransform()->GetScale();

				ImGui::DragFloat3("Scale", reinterpret_cast<float*>(&entityScales[i]), 0.1f);

				entities[i]->GetTransform()->SetScale(entityScales[i]);
				ImGui::TreePop();
			}

		}
	}
	if (ImGui::CollapsingHeader("Lights"))
	{
		for (unsigned int i = 0; i < lights.size(); i++)
		{

			switch (lights[i].type)
			{
				case 0:
					if (ImGui::TreeNode(("Directional Light " + std::to_string(i + 1)).c_str()))
					{
						ImGui::DragFloat3("Direction", reinterpret_cast<float*>(&lights[i].direction), 0.1f);

						ImGui::DragFloat3("Color", reinterpret_cast<float*>(&lights[i].color), 0.1f);

						ImGui::DragFloat("Intensity", reinterpret_cast<float*>(&lights[i].intensity), 0.1f);

						if (i == 0)
						{
							//normalize direction
							DirectX::XMVECTOR direction = DirectX::XMLoadFloat3(&lights[0].direction);
							DirectX::XMVECTOR normalizedDirection = DirectX::XMVector3Normalize(direction);
							DirectX::XMFLOAT3 normalizedDirectionFloat3;
							DirectX::XMStoreFloat3(&normalizedDirectionFloat3, normalizedDirection);

							XMMATRIX lightView = XMMatrixLookToLH(
							-normalizedDirection * 20, // Position: "Backing up" 20 units from origin
							normalizedDirection, // Direction: light's direction
							XMVectorSet(0, 1, 0, 0)); // Up: World up vector (Y axis)

							lightViewMatrix = lightView;
						}
						
						ImGui::TreePop();
					}
					break;

				case 1:
					if (ImGui::TreeNode(("Point Light " + std::to_string(i + 1)).c_str()))
					{
						ImGui::DragFloat3("Position", reinterpret_cast<float*>(&lights[i].position), 0.1f);

						ImGui::DragFloat3("Color", reinterpret_cast<float*>(&lights[i].color), 0.1f);

						ImGui::DragFloat("Intensity", reinterpret_cast<float*>(&lights[i].intensity), 0.1f);

						ImGui::DragFloat("Range", reinterpret_cast<float*>(&lights[i].range), 0.1f);

						ImGui::TreePop();
					}
					break;

				case 2:
					break;
			}

		}
	}
	if (ImGui::CollapsingHeader("Cameras"))
	{
		if (ImGui::Button("Next Camera"))
		{
			if (activeCameraIndex == numCameras - 1)
			{
				activeCameraIndex = 0;
			}
			else
			{
				activeCameraIndex++;
			}
		}
		else if (ImGui::Button("Previous Camera"))
		{
			if (activeCameraIndex == 0)
			{
				activeCameraIndex = numCameras - 1;
			}
			else
			{
				activeCameraIndex--;
			}
		}

		ImGui::Text("Active Camera: ");
		ImGui::Text("Position: (%f, %f, %f)", cameras[activeCameraIndex]->GetTransform()->GetPosition().x,
			cameras[activeCameraIndex]->GetTransform()->GetPosition().y, cameras[activeCameraIndex]->GetTransform()->GetPosition().z);
		ImGui::Text("Facing: (%f, %f, %f)", cameras[activeCameraIndex]->GetTransform()->GetForwardVector().x,
			cameras[activeCameraIndex]->GetTransform()->GetForwardVector().y, cameras[activeCameraIndex]->GetTransform()->GetForwardVector().z);
	}
	if (ImGui::CollapsingHeader("Post Processing Options"))
	{
		ImGui::SliderInt("Blur Radius", &blurRadius, 0, 200);
		ImGui::SliderInt("Pixel Size", &pixelSize, 1, 200);
		ImGui::Checkbox("Posterize", &posterize);
		if (posterize)
		{
			ImGui::SliderFloat("Posterization Level", &posterizeLevel, 1, 100);
		}
	}

	ImGui::End();
}

// --------------------------------------------------------
// Author: Chris Cascioli
// Purpose: Creates a cube map on the GPU from 6 individual textures
// 
// - You are allowed to directly copy/paste this into your code base
//   for assignments, given that you clearly cite that this is not
//   code of your own design.
//
// - Note: This code assumes you�re putting the function in Sky.cpp, 
//   you�ve included WICTextureLoader.h and you have an ID3D11Device 
//   ComPtr called �device�.  Make any adjustments necessary for
//   your own implementation.
// --------------------------------------------------------
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Game::CreateCubemap(
	const wchar_t* right,
	const wchar_t* left,
	const wchar_t* up,
	const wchar_t* down,
	const wchar_t* front,
	const wchar_t* back)
{
	// Load the 6 textures into an array.
	// - We need references to the TEXTURES, not SHADER RESOURCE VIEWS!
	// - Explicitly NOT generating mipmaps, as we don't need them for the sky!
	// - Order matters here!  +X, -X, +Y, -Y, +Z, -Z
	Microsoft::WRL::ComPtr<ID3D11Texture2D> textures[6] = {};
	CreateWICTextureFromFile(device.Get(), right, (ID3D11Resource**)textures[0].GetAddressOf(), 0);
	CreateWICTextureFromFile(device.Get(), left, (ID3D11Resource**)textures[1].GetAddressOf(), 0);
	CreateWICTextureFromFile(device.Get(), up, (ID3D11Resource**)textures[2].GetAddressOf(), 0);
	CreateWICTextureFromFile(device.Get(), down, (ID3D11Resource**)textures[3].GetAddressOf(), 0);
	CreateWICTextureFromFile(device.Get(), front, (ID3D11Resource**)textures[4].GetAddressOf(), 0);
	CreateWICTextureFromFile(device.Get(), back, (ID3D11Resource**)textures[5].GetAddressOf(), 0);

	// We'll assume all of the textures are the same color format and resolution,
	// so get the description of the first texture
	D3D11_TEXTURE2D_DESC faceDesc = {};
	textures[0]->GetDesc(&faceDesc);

	// Describe the resource for the cube map, which is simply 
	// a "texture 2d array" with the TEXTURECUBE flag set.  
	// This is a special GPU resource format, NOT just a 
	// C++ array of textures!!!
	D3D11_TEXTURE2D_DESC cubeDesc = {};
	cubeDesc.ArraySize = 6;            // Cube map!
	cubeDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE; // We'll be using as a texture in a shader
	cubeDesc.CPUAccessFlags = 0;       // No read back
	cubeDesc.Format = faceDesc.Format; // Match the loaded texture's color format
	cubeDesc.Width = faceDesc.Width;   // Match the size
	cubeDesc.Height = faceDesc.Height; // Match the size
	cubeDesc.MipLevels = 1;            // Only need 1
	cubeDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE; // This should be treated as a CUBE, not 6 separate textures
	cubeDesc.Usage = D3D11_USAGE_DEFAULT; // Standard usage
	cubeDesc.SampleDesc.Count = 1;
	cubeDesc.SampleDesc.Quality = 0;

	// Create the final texture resource to hold the cube map
	Microsoft::WRL::ComPtr<ID3D11Texture2D> cubeMapTexture;
	device->CreateTexture2D(&cubeDesc, 0, cubeMapTexture.GetAddressOf());

	// Loop through the individual face textures and copy them,
	// one at a time, to the cube map texure
	for (int i = 0; i < 6; i++)
	{
		// Calculate the subresource position to copy into
		unsigned int subresource = D3D11CalcSubresource(
			0,  // Which mip (zero, since there's only one)
			i,  // Which array element?
			1); // How many mip levels are in the texture?

		// Copy from one resource (texture) to another
		context->CopySubresourceRegion(
			cubeMapTexture.Get(),  // Destination resource
			subresource,           // Dest subresource index (one of the array elements)
			0, 0, 0,               // XYZ location of copy
			textures[i].Get(),     // Source resource
			0,                     // Source subresource index (we're assuming there's only one)
			0);                    // Source subresource "box" of data to copy (zero means the whole thing)
	}

	// At this point, all of the faces have been copied into the 
	// cube map texture, so we can describe a shader resource view for it
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = cubeDesc.Format;         // Same format as texture
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE; // Treat this as a cube!
	srvDesc.TextureCube.MipLevels = 1;        // Only need access to 1 mip
	srvDesc.TextureCube.MostDetailedMip = 0;  // Index of the first mip we want to see

	// Make the SRV
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cubeSRV;
	device->CreateShaderResourceView(cubeMapTexture.Get(), &srvDesc, cubeSRV.GetAddressOf());

	// Send back the SRV, which is what we need for our shaders
	return cubeSRV;
}
