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

		void CreateBuffers(Microsoft::WRL::ComPtr<ID3D11Device> device, Vertex* vertices, unsigned int vertexCount, unsigned int* indices);

		void CalculateTangents(Vertex* verts, int numVerts, unsigned int* indices, int numIndices);

	public:

		Mesh(Microsoft::WRL::ComPtr<ID3D11Device> device, Vertex* vertices, int vertexCount, unsigned int* indices, int indexCount);

		Mesh(Microsoft::WRL::ComPtr<ID3D11Device> device, const wchar_t* fileName);
		
		~Mesh();

		Microsoft::WRL::ComPtr<ID3D11Buffer> GetVertexBuffer();
		
		Microsoft::WRL::ComPtr<ID3D11Buffer> GetIndexBuffer();
		
		int GetIndexCount();
		
		void Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context);


};

