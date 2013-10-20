
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
}

BasicEffect::~BasicEffect()
{
}

std::unique_ptr<BasicEffect> Effects::BasicFX = nullptr;

BlurEffect::BlurEffect(ID3D11Device* device, const std::string& filename)
	: Effect(device, filename)
{
	HorzBlurTech = m_fx->GetTechniqueByName("HorzBlur");
	VertBlurTech = m_fx->GetTechniqueByName("VertBlur");

	Weights     = m_fx->GetVariableByName("gWeights")->AsScalar();
	InputMap    = m_fx->GetVariableByName("gInput")->AsShaderResource();
	OutputMap   = m_fx->GetVariableByName("gOutput")->AsUnorderedAccessView();
}

BlurEffect::~BlurEffect() {}

std::unique_ptr<BlurEffect> Effects::BlurFX = nullptr;


TessellationEffect::TessellationEffect(ID3D11Device* device, const std::string& filename)
	: Effect(device, filename)
{
	TessTech = m_fx->GetTechniqueByName("Tess");

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
}

TessellationEffect::~TessellationEffect() { }

std::unique_ptr<TessellationEffect> Effects::TessellationFX = nullptr;


void Effects::InitAll(ID3D11Device* device)
{
    BasicFX.reset(new BasicEffect(device, "FX/Basic.fxo"));
    BlurFX.reset(new BlurEffect(device, "FX/Blur.fxo"));
    TessellationFX.reset(new TessellationEffect(device, "FX/Tessellation.fxo"));
}

void Effects::DestroyAll()
{
    BasicFX.reset(nullptr);
    BlurFX.reset(nullptr);
    TessellationFX.reset(nullptr);
}