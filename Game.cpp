#include "Game.h"
#include "Vertex.h"
#include "Input.h"
#include "PathHelpers.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"
#include "Transform.h"
#include <iostream>


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

	// Helper methods for loading shaders, creating some basic
	// geometry to draw and some simple camera matrices.
	//  - You'll be expanding and/or replacing these later
	LoadShaders();
	CreateGeometry();
	
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
	ps = std::make_shared<SimplePixelShader>(device, context, FixPath(L"PixelShader.cso").c_str());
	vs = std::make_shared<SimpleVertexShader>(device, context, FixPath(L"VertexShader.cso").c_str());

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



// --------------------------------------------------------
// Creates the geometry we're going to draw - a single triangle for now
// --------------------------------------------------------
void Game::CreateGeometry()
{
	//clean up empty meshes before reassignment
	delete[] meshes;
	meshCount = 4;
	entityCount = 7;
	meshes = new std::shared_ptr<Mesh>[entityCount];
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
	XMFLOAT4 red	= XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	XMFLOAT4 green	= XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	XMFLOAT4 blue	= XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	XMFLOAT4 black = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	XMFLOAT4 white = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	//Triangle
	{
		Vertex vertices[] =
		{
			{ XMFLOAT3(+0.0f, +0.5f, +0.0f), red },
			{ XMFLOAT3(+0.5f, -0.5f, +0.0f), blue },
			{ XMFLOAT3(-0.5f, -0.5f, +0.0f), green },
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
			{ XMFLOAT3(-0.5f, +0.7f, +0.0f), green },
			{ XMFLOAT3(-0.2f, +0.7f, +0.0f), blue },
			{ XMFLOAT3(-0.2f, +0.2f, +0.0f), blue },
			{ XMFLOAT3(-0.5f, +0.7f, +0.0f), green },
			{ XMFLOAT3(-0.2f, +0.2f, +0.0f), blue },
			{ XMFLOAT3(-0.5f, +0.2f, +0.0f), green }
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
			{ XMFLOAT3(+0.5f, +0.7f, +0.0f), red },
			{ XMFLOAT3(+0.55f, +0.5f, +0.0f), white },
			{ XMFLOAT3(+0.35f, +0.5f, +0.0f), blue },
			{ XMFLOAT3(+0.6f, +0.7f, +0.0f), red },
			{ XMFLOAT3(+0.75f, +0.5f, +0.0f), blue },
			{ XMFLOAT3(+0.55f, +0.5f, +0.0f), white },
			{ XMFLOAT3(+0.35f, +0.5f, +0.0f), blue },
			{ XMFLOAT3(+0.55f, +0.5f, +0.0f), white },
			{ XMFLOAT3(+0.45f, +0.0f, +0.0f), green },
			{ XMFLOAT3(+0.55f, +0.5f, +0.0f), white },
			{ XMFLOAT3(+0.75f, +0.5f, +0.0f), blue },
			{ XMFLOAT3(+0.65f, +0.0f, +0.0f), green },
		};
		// Set up indices, which tell us which vertices to use and in which order
		// - This is redundant for just 3 vertices, but will be more useful later
		// - Indices are technically not required if the vertices are in the buffer 
		//    in the correct order and each one will be used exactly once
		// - But just to see how it's done...
		unsigned int indices[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
		std::shared_ptr<Mesh> butterfly; // Declaration (probably in a header)
		butterfly = std::make_shared<Mesh>(device, vertices, 12, indices, 12); // Initialization

		meshes[2] = butterfly;
	}
	//Cube
	{
		Vertex vertices[] =
		{
			{ XMFLOAT3(+0.5f, +0.5f, -0.5f), red },
			{ XMFLOAT3(+0.5f, -0.5f, -0.5f), blue },
			{ XMFLOAT3(-0.5f, -0.5f, -0.5f), red },
			{ XMFLOAT3(-0.5f, -0.5f, -0.5f), blue },
			{ XMFLOAT3(-0.5f, +0.5f, -0.5f), red },
			{ XMFLOAT3(+0.5f, +0.5f, -0.5f), blue },

			{ XMFLOAT3(+0.5f, +0.5f, -0.5f), red },
			{ XMFLOAT3(+0.5f, +0.5f, +0.5f), blue },
			{ XMFLOAT3(+0.5f, -0.5f, -0.5f), red },
			{ XMFLOAT3(+0.5f, +0.5f, +0.5f), blue },
			{ XMFLOAT3(+0.5f, -0.5f, +0.5f), red },
			{ XMFLOAT3(+0.5f, -0.5f, -0.5f), blue },


			{ XMFLOAT3(-0.5f, -0.5f, -0.5f), red },
			{ XMFLOAT3(-0.5f, +0.5f, +0.5f), blue },
			{ XMFLOAT3(-0.5f, +0.5f, -0.5f), red },
			{ XMFLOAT3(-0.5f, -0.5f, -0.5f), blue },
			{ XMFLOAT3(-0.5f, -0.5f, +0.5f), red },
			{ XMFLOAT3(-0.5f, +0.5f, +0.5f), blue },

			{ XMFLOAT3(-0.5f, -0.5f, +0.5f), red },
			{ XMFLOAT3(+0.5f, -0.5f, +0.5f), blue },
			{ XMFLOAT3(+0.5f, +0.5f, +0.5f), red },
			{ XMFLOAT3(+0.5f, +0.5f, +0.5f), blue },
			{ XMFLOAT3(-0.5f, +0.5f, +0.5f), red },
			{ XMFLOAT3(-0.5f, -0.5f, +0.5f), blue },

			{ XMFLOAT3(-0.5f, +0.5f, +0.5f), red },
			{ XMFLOAT3(+0.5f, +0.5f, -0.5f), blue },
			{ XMFLOAT3(-0.5f, +0.5f, -0.5f), red },
			{ XMFLOAT3(-0.5f, +0.5f, +0.5f), blue },
			{ XMFLOAT3(+0.5f, +0.5f, +0.5f), red },
			{ XMFLOAT3(+0.5f, +0.5f, -0.5f), blue },

			{ XMFLOAT3(-0.5f, -0.5f, -0.5f), red },
			{ XMFLOAT3(+0.5f, -0.5f, -0.5f), blue },
			{ XMFLOAT3(-0.5f, -0.5f, +0.5f), red },
			{ XMFLOAT3(+0.5f, -0.5f, -0.5f), blue },
			{ XMFLOAT3(+0.5f, -0.5f, +0.5f), red },
			{ XMFLOAT3(-0.5f, -0.5f, +0.5f), blue },
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
	
	
	for (unsigned int i = 0; i < meshCount; i++)
	{
		entities[i] = std::make_shared<GameEntity>(meshes[i]);
	}

	entities[meshCount] = std::make_shared<GameEntity>(meshes[1]);
	entities[meshCount + 1] = std::make_shared<GameEntity>(meshes[1]);
	entities[meshCount + 2] = std::make_shared<GameEntity>(meshes[2]);

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
	for(unsigned int i = 0; i < numCameras; i++)
	cameras[i]->UpdateProjectionMatrix(float(windowWidth) / float(windowHeight));
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

		// Clear the depth buffer (resets per-pixel occlusion information)
		context->ClearDepthStencilView(depthBufferDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	}


	
	//XMStoreFloat4x4(&vsData.world, XMMatrixIdentity());
	//vsData.offset = { offsetUI[0], offsetUI[1], offsetUI[2] };

	//insert mesh drawing here
	for (unsigned int i = 0; i < entityCount; i++)
	{
		entities[i]->Draw(context, vs, ps, cameras[activeCameraIndex], totalTime);
	}
	// Frame END
	// - These should happen exactly ONCE PER FRAME
	// - At the very end of the frame (after drawing *everything*)
	{
		// Present the back buffer to the user
		//  - Puts the results of what we've drawn onto the window
		//  - Without this, the user never sees anything
		bool vsyncNecessary = vsync || !deviceSupportsTearing || isFullscreen;

		ImGui::Render(); // Turns this frame’s UI into renderable triangles
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData()); // Draws it to the screen

		swapChain->Present(
			vsyncNecessary ? 1 : 0,
			vsyncNecessary ? 0 : DXGI_PRESENT_ALLOW_TEARING);

		// Must re-bind buffers after presenting, as they become unbound
		context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthBufferDSV.Get());
	}
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
	if(showWindow)
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
				ImGui::Begin(("New Window " + std::to_string(i+1)).c_str());
			}
			ImGui::Text("I'm a window!");
			ImGui::End();
		}
	}
	if (ImGui::CollapsingHeader("Meshes"))
	{
		for (unsigned int i = 0; i < meshCount; i++)
		{
			ImGui::Text("Mesh %d: %d triangle(s)", i, meshes[i]->GetIndexCount()/3);
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
	ImGui::End();
}
