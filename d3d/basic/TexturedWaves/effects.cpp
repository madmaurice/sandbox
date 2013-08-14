
#include "effects.h"
#include "config.h"
#include <vector>
#include <fstream>

Effect::Effect(ID3D11Device* device, const std::string& filename)
: m_fx(nullptr)
{
	std::ifstream fin(filename, std::ios::binary);
    OC_ASSERT(fin.good());

	fin.seekg(0, std::ios_base::end);
	int size = (int)fin.tellg();
	fin.seekg(0, std::ios_base::beg);
	std::vector<char> compiledShader(size);

	fin.read(&compiledShader[0], size);
	fin.close();
	
	HR(D3DX11CreateEffectFromMemory(&compiledShader[0], size, 
        0, device, m_fx.GetAddressOf()));
}

Effect::~Effect()
{
}

BasicEffect::BasicEffect(ID3D11Device* device, const std::string& filename)
: Effect(device, filename)
{
	Light1Tech        = m_fx->GetTechniqueByName("Light1");
	Light2Tech        = m_fx->GetTechniqueByName("Light2");
	Light3Tech        = m_fx->GetTechniqueByName("Light3");

   	Light0TexTech = m_fx->GetTechniqueByName("Light0Tex");
	Light1TexTech = m_fx->GetTechniqueByName("Light1Tex");
	Light2TexTech = m_fx->GetTechniqueByName("Light2Tex");
	Light3TexTech = m_fx->GetTechniqueByName("Light3Tex");

	WorldViewProj     = m_fx->GetVariableByName("gWorldViewProj")->AsMatrix();
	World             = m_fx->GetVariableByName("gWorld")->AsMatrix();
	WorldInvTranspose = m_fx->GetVariableByName("gWorldInvTranspose")->AsMatrix();
    TexTransform      = m_fx->GetVariableByName("gTexTransform")->AsMatrix();
	EyePosW           = m_fx->GetVariableByName("gEyePosW")->AsVector();
	DirLights         = m_fx->GetVariableByName("gDirLights");
	Mat               = m_fx->GetVariableByName("gMaterial");
    DiffuseMap        = m_fx->GetVariableByName("gDiffuseMap")->AsShaderResource();
}

BasicEffect::~BasicEffect()
{
}

std::unique_ptr<BasicEffect> Effects::BasicFX = nullptr;

void Effects::InitAll(ID3D11Device* device)
{
    BasicFX.reset(new BasicEffect(device, "FX/Basic.fxo"));
}

void Effects::DestroyAll()
{
    BasicFX.reset(nullptr);
}