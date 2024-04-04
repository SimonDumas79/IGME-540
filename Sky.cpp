#include "Sky.h"

Sky::Sky(std::shared_ptr<Mesh> mesh, 
	Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler, 
	Microsoft::WRL::ComPtr<ID3D11Device> device,
	std::shared_ptr<SimpleVertexShader> vertexShader,
	std::shared_ptr<SimplePixelShader> pixelShader):
	mesh(mesh),
	sampler(sampler),
	vs(vertexShader),
	ps(pixelShader)
{
	
	D3D11_RASTERIZER_DESC rasterizerDesc = {};
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_FRONT;
	device->CreateRasterizerState(&rasterizerDesc, &rasterizerState);

	D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	device->CreateDepthStencilState(&depthStencilDesc, &depthStencilState);

}

Sky::~Sky()
{

}

void Sky::SetShaderResourceView(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv)
{
	this->srv = srv;
}

void Sky::Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, std::shared_ptr<Camera> camera)
{
	context->RSSetState(rasterizerState.Get());
	context->OMSetDepthStencilState(depthStencilState.Get(), 0);

	ps->SetShader();
	vs->SetShader();

	ps->SetShaderResourceView("SkyCube", srv);
	ps->SetSamplerState("BasicSampler", sampler);

	vs->SetMatrix4x4("viewMatrix", camera->GetView());
	vs->SetMatrix4x4("projectionMatrix", camera->GetProjection());

	vs->CopyAllBufferData();
	ps->CopyAllBufferData();

	mesh->Draw(context);

	context->RSSetState(0);
	context->OMSetDepthStencilState(0, 0);
}
