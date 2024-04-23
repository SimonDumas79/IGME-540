#include "Material.h"

Material::Material(DirectX::XMFLOAT4 colorTint, 
    std::shared_ptr<SimpleVertexShader> vertexShader, 
    std::shared_ptr<SimplePixelShader> pixelShader,
    float roughness,
    bool PBR) :
    colorTint(colorTint),
    vs(vertexShader),
    ps(pixelShader),
    roughness(roughness),
    PBR(PBR)
{
}

Material::~Material()
{
}

void Material::SetColorTint(DirectX::XMFLOAT4 colorTint)
{
    this->colorTint = colorTint;
}

DirectX::XMFLOAT4 Material::GetColorTint()
{
    return colorTint;
}

void Material::SetVertexShader(std::shared_ptr<SimpleVertexShader> vertexShader)
{
    vs.reset();
    vs = vertexShader;
}

std::shared_ptr<SimpleVertexShader> Material::GetVertexShader()
{
    return vs;
}

void Material::SetPixelShader(std::shared_ptr<SimplePixelShader> pixelShader)
{
    ps.reset();
    ps = pixelShader;
}

std::shared_ptr<SimplePixelShader> Material::GetPixelShader()
{
    return ps;
}

void Material::AddTextureSRV(std::string shaderVariableName, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv)
{
    textureSRVs.insert({ shaderVariableName, srv });
}

void Material::AddSampler(std::string shaderVariableName, Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState)
{
    samplers.insert({ shaderVariableName, samplerState });
}

//bind the srvs and samplers before drawing
void Material::PrepareMaterial()
{
    for (auto& t : textureSRVs) { ps->SetShaderResourceView(t.first.c_str(), t.second); }
    for (auto& s : samplers) { ps->SetSamplerState(s.first.c_str(), s.second); }
}


void Material::SetRoughness(float roughness)
{
    this->roughness = roughness;
}

float Material::GetRoughness()
{
    return roughness;
}

bool Material::GetPBR()
{
    return PBR;
}
