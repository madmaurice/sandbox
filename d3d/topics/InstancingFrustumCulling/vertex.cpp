#include "vertex.h"
#include "effects.h"

const D3D11_INPUT_ELEMENT_DESC InputLayoutDesc::InstancedBasic32[8] = 
{
	{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	{"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
	{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
	{ "WORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
    { "WORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
    { "WORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
    { "WORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
	{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 64,  D3D11_INPUT_PER_INSTANCE_DATA, 1 }
};

ComPtr<ID3D11InputLayout> InputLayouts::InstancedBasic32 = nullptr;

void InputLayouts::InitAll(ID3D11Device* device)
{
    //Note: The input layout can be created with any tech, what matters is the shader associated
    //      with the pass desc (IAInputSignature -> Input Assembler Input Signature)

	// InstancedBasic32
	D3DX11_PASS_DESC passDescBasic;
	Effects::InstancedBasicFX->Light1Tech->GetPassByIndex(0)->GetDesc(&passDescBasic);
	HR(device->CreateInputLayout(InputLayoutDesc::InstancedBasic32, 8, passDescBasic.pIAInputSignature, 
        passDescBasic.IAInputSignatureSize, InstancedBasic32.GetAddressOf()));
}

void InputLayouts::DestroyAll()
{
}

