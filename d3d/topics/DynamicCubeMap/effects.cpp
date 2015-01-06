
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

	Light1ReflectTech    = m_fx->GetTechniqueByName("Light1Reflect");
	Light2ReflectTech    = m_fx->GetTechniqueByName("Light2Reflect");
	Light3ReflectTech    = m_fx->GetTechniqueByName("Light3Reflect");

	Light0TexReflectTech = m_fx->GetTechniqueByName("Light0TexReflect");
	Light1TexReflectTech = m_fx->GetTechniqueByName("Light1TexReflect");
	Light2TexReflectTech = m_fx->GetTechniqueByName("Light2TexReflect");
	Light3TexReflectTech = m_fx->GetTechniqueByName("Light3TexReflect");

	Light0TexAlphaClipReflectTech = m_fx->GetTechniqueByName("Light0TexAlphaClipReflect");
	Light1TexAlphaClipReflectTech = m_fx->GetTechniqueByName("Light1TexAlphaClipReflect");
	Light2TexAlphaClipReflectTech = m_fx->GetTechniqueByName("Light2TexAlphaClipReflect");
	Light3TexAlphaClipReflectTech = m_fx->GetTechniqueByName("Light3TexAlphaClipReflect");

	Light1FogReflectTech    = m_fx->GetTechniqueByName("Light1FogReflect");
	Light2FogReflectTech    = m_fx->GetTechniqueByName("Light2FogReflect");
	Light3FogReflectTech    = m_fx->GetTechniqueByName("Light3FogReflect");

	Light0TexFogReflectTech = m_fx->GetTechniqueByName("Light0TexFogReflect");
	Light1TexFogReflectTech = m_fx->GetTechniqueByName("Light1TexFogReflect");
	Light2TexFogReflectTech = m_fx->GetTechniqueByName("Light2TexFogReflect");
	Light3TexFogReflectTech = m_fx->GetTechniqueByName("Light3TexFogReflect");

	Light0TexAlphaClipFogReflectTech = m_fx->GetTechniqueByName("Light0TexAlphaClipFogReflect");
	Light1TexAlphaClipFogReflectTech = m_fx->GetTechniqueByName("Light1TexAlphaClipFogReflect");
	Light2TexAlphaClipFogReflectTech = m_fx->GetTechniqueByName("Light2TexAlphaClipFogReflect");
	Light3TexAlphaClipFogReflectTech = m_fx->GetTechniqueByName("Light3TexAlphaClipFogReflect");

	WorldViewProj     = m_fx->GetVariableByName("gWorldViewProj")->AsMatrix();
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
    CubeMap           = m_fx->GetVariableByName("gCubeMap")->AsShaderResource();
}

BasicEffect::~BasicEffect()
{
}

std::unique_ptr<BasicEffect> Effects::BasicFX = nullptr;

SkyEffect::SkyEffect(ID3D11Device* device, const std::string& filename)
	: Effect(device, filename)
{
	SkyTech       = m_fx->GetTechniqueByName("SkyTech");
	WorldViewProj = m_fx->GetVariableByName("gWorldViewProj")->AsMatrix();
	CubeMap       = m_fx->GetVariableByName("gCubeMap")->AsShaderResource();
}

SkyEffect::~SkyEffect()
{
}

std::unique_ptr<SkyEffect> Effects::SkyFX = nullptr;

void Effects::InitAll(ID3D11Device* device)
{
    BasicFX.reset(new BasicEffect(device, "FX/Basic.fxo"));
    SkyFX.reset(new SkyEffect(device, "FX/Sky.fxo"));
}

void Effects::DestroyAll()
{
    BasicFX.reset(nullptr);
    SkyFX.reset(nullptr);
}