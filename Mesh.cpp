#include "Mesh.h"

Mesh::Mesh(Microsoft::WRL::ComPtr<ID3D11Device> device, Vertex* vertices, int vertexCount, unsigned int* indices, int indexCount)
{
	this->indexCount = indexCount;
	this->context = context;
	
	//Vertex Buffer
	// Create the buffer description.
	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE; // no changing after it reaches GPU.
	vbd.ByteWidth = sizeof(Vertex) * vertexCount; // bytewidth = number of verts * size of single vert.
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;

	// Create the proper struct to hold the initial vertex data
	D3D11_SUBRESOURCE_DATA initialVertexData;
	initialVertexData.pSysMem = vertices;

	// Create the actual buffer using the device. Pass items in by reference.
	device->CreateBuffer(&vbd, &initialVertexData, &vertexBuffer);


	//Index Buffer
	// Create the buffer description.
	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(unsigned int) * indexCount; 
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	ibd.StructureByteStride = 0;

	// Create the proper struct to hold the initial index data
	D3D11_SUBRESOURCE_DATA initialIndexData;
	initialIndexData.pSysMem = indices;

	//create the buffer
	device->CreateBuffer(&ibd, &initialIndexData, &indexBuffer);
}


Mesh::~Mesh()
{
	vertexBuffer.Reset();
	indexBuffer.Reset();
}

Microsoft::WRL::ComPtr<ID3D11Buffer> Mesh::GetVertexBuffer()
{
	return vertexBuffer;
}

Microsoft::WRL::ComPtr<ID3D11Buffer> Mesh::GetIndexBuffer()
{
	return indexBuffer;
}

int Mesh::GetIndexCount()
{
	return indexCount;
}

void Mesh::Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext>	context)
{

	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	//set the vertex buffer
	context->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
	//set the index buffer
	context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	//call to draw mesh
	context->DrawIndexed(
		indexCount,     // The number of indices to use (we could draw a subset if we wanted)
		0,     // Offset to the first index we want to use
		0);    // Offset to add to each index when looking up vertices

}
