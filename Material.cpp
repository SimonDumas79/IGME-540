#include "Material.h"

Material::Material(DirectX::XMFLOAT4 colorTint, 
    std::shared_ptr<SimpleVertexShader> vertexShader, 
    std::shared_ptr<SimplePixelShader> pixelShader) :
    colorTint(colorTint),
    vs(vertexShader),
    ps(pixelShader)
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
