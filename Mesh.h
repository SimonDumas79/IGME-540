#pragma once

#include <wrl/client.h> 
#include <d3d11.h>
#include <DirectXMath.h>
#include "Vertex.h"

class Mesh
{
	private:
		
		Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;

		int indexCount;

		Microsoft::WRL::ComPtr<ID3D11DeviceContext>	context;


	public:

		Mesh(Microsoft::WRL::ComPtr<ID3D11Device> device, Vertex* vertices, int vertexCount, unsigned int* indices, int indexCount);
		Mesh();

		~Mesh();

		Microsoft::WRL::ComPtr<ID3D11Buffer> GetVertexBuffer();
		
		Microsoft::WRL::ComPtr<ID3D11Buffer> GetIndexBuffer();
		
		int GetIndexCount();
		
		void Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context);


};

