#include "vertex.h"
#include "effects.h"


const D3D11_INPUT_ELEMENT_DESC InputLayoutDesc::Basic32[3] = 
{
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	{"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
	{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0}
};

ComPtr<ID3D11InputLayout> InputLayouts::Basic32 = nullptr;

void InputLayouts::InitAll(ID3D11Device* device)
{
    //Note: The input layout can be created with any tech, what matters is the shader associated
    //      with the pass desc (IAInputSignature -> Input Assembler Input Signature)

	// Basic32
	D3DX11_PASS_DESC passDescBasic;
	Effects::BasicFX->Light1Tech->GetPassByIndex(0)->GetDesc(&passDescBasic);
	HR(device->CreateInputLayout(InputLayoutDesc::Basic32, 3, passDescBasic.pIAInputSignature, 
        passDescBasic.IAInputSignatureSize, Basic32.GetAddressOf()));
}

void InputLayouts::DestroyAll()
{
}

