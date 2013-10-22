
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

InstancedBasicEffect::InstancedBasicEffect(ID3D11Device* device, const std::string& filename)
	: Effect(device, filename)
{
	Light1Tech    = m_fx->GetTechniqueByName("Light1");
	Light2Tech    = m_fx->GetTechniqueByName("Light2");
	Light3Tech    = m_fx->GetTechniqueByName("Light3");

	Light0TexTech = m_fx->GetTechniqueByName("Light0Tex");
	Light1TexTech = m_fx->GetTechniqueByName("Light1Tex");
	Light2TexTech = m_fx->GetTechniqueByName("Light2Tex");
	Light3TexTech = m_fx->GetTechniqueByName("Light3Tex");

	Light0TexAlphaClipTech = m_fx->GetTechniqueByName("Light0TexAlphaClip");
	Light1TexAlphaClipTech = m_fx->GetTechniqueByName("Light1TexAlphaClip");
	Light2TexAlphaClipTech = m_fx->GetTechniqueByName("Light2TexAlphaClip");
	Light3TexAlphaClipTech = m_fx->GetTechniqueByName("Light3TexAlphaClip");

	Light1FogTech    = m_fx->GetTechniqueByName("Light1Fog");
	Light2FogTech    = m_fx->GetTechniqueByName("Light2Fog");
	Light3FogTech    = m_fx->GetTechniqueByName("Light3Fog");

	Light0TexFogTech = m_fx->GetTechniqueByName("Light0TexFog");
	Light1TexFogTech = m_fx->GetTechniqueByName("Light1TexFog");
	Light2TexFogTech = m_fx->GetTechniqueByName("Light2TexFog");
	Light3TexFogTech = m_fx->GetTechniqueByName("Light3TexFog");

	Light0TexAlphaClipFogTech = m_fx->GetTechniqueByName("Light0TexAlphaClipFog");
	Light1TexAlphaClipFogTech = m_fx->GetTechniqueByName("Light1TexAlphaClipFog");
	Light2TexAlphaClipFogTech = m_fx->GetTechniqueByName("Light2TexAlphaClipFog");
	Light3TexAlphaClipFogTech = m_fx->GetTechniqueByName("Light3TexAlphaClipFog");

	ViewProj          = m_fx->GetVariableByName("gViewProj")->AsMatrix();
	World             = m_fx->GetVariableByName("gWorld")->AsMatrix();
	WorldInvTranspose = m_fx->GetVariableByName("gWorldInvTranspose")->AsMatrix();
	TexTransform      = m_fx->GetVariableByName("gTexTransform")->AsMatrix();
	EyePosW           = m_fx->GetVariableByName("gEyePosW")->AsVector();
	FogColor          = m_fx->GetVariableByName("gFogColor")->AsVector();
	FogStart          = m_fx->GetVariableByName("gFogStart")->AsScalar();
	FogRange          = m_fx->GetVariableByName("gFogRange")->AsScalar();
	DirLights         = m_fx->GetVariableByName("gDirLights");
	Mat               = m_fx->GetVariableByName("gMaterial");
	DiffuseMap        = m_fx->GetVariableByName("gDiffuseMap")->AsShaderResource();
}

InstancedBasicEffect::~InstancedBasicEffect()
{
}

std::unique_ptr<InstancedBasicEffect> Effects::InstancedBasicFX = nullptr;

void Effects::InitAll(ID3D11Device* device)
{
    InstancedBasicFX.reset(new InstancedBasicEffect(device, "FX/InstancedBasic.fxo"));
}

void Effects::DestroyAll()
{
    InstancedBasicFX.reset(nullptr);
}